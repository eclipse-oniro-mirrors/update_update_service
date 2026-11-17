/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "firmware_stream_installer_install.h"

#include <dirent.h>
#include <iostream>
#include <unistd.h>

#include "isys_installer.h"
#include "sys_installer_kits_impl.h"

#include "config_parse.h"
#include "dupdate_errno.h"
#include "firmware_constant.h"
#include "firmware_log.h"
#include "firmware_sys_installer_callback.h"
#include "firmware_update_helper.h"

namespace OHOS {
namespace UpdateService {

bool StreamInstallerInstall::IsComponentLegal(const std::vector<FirmwareComponent> &componentList)
{
    return true;
}

bool StreamInstallerInstall::PerformInstall(const std::vector<FirmwareComponent> &componentList, UpgradeStatus &status)
{
    FIRMWARE_LOGI("StreamInstallerInstall::PerformInstall");
    if (componentList.empty()) {
        return false;
    }
    uint32_t successCount = 0;
    for (const auto &component : componentList) {
        if (onInstallCallback_.onFirmwareStatus == nullptr) {
            FIRMWARE_LOGE("StreamInstallerInstall PerformInstall onFirmwareStatus is null");
            continue;
        }
        onInstallCallback_.onFirmwareStatus(UpgradeStatus::INSTALLING);
        if (InstallComponentWithRetry(component)) {
            successCount++;
        }
    }
    if (sysInstallProgress_.status != UpgradeStatus::INSTALL_FAIL) {
        status = downloadProgress_.status;
    }
    
    return successCount == static_cast<uint32_t>(componentList.size());
}

bool StreamInstallerInstall::InstallComponentWithRetry(const FirmwareComponent &component)
{
    int16_t retry = 0;
    while (retry < MAX_RETRY_COUNT) {
        FIRMWARE_LOGI("InstallComponentWithRetry PerformInstall retry = %{public}d", retry);

        FirmwareComponent refreshComponent;
        FirmwareComponentOperator().QueryByUrl(component.url, refreshComponent);

        if (DoSysInstall(refreshComponent) == OHOS_SUCCESS) {
            return true;
        } else {
            if (downloadProgress_.status == UpgradeStatus::DOWNLOAD_CANCEL) {
                return false;
            }
        }

        retry++;
        usleep(RETRY_INTERVAL_TIME);
    }

    return false;
}

int32_t StreamInstallerInstall::DoSysInstall(const FirmwareComponent &component)
{
    #ifndef UPDATER_UT
    SysInstaller::SysInstallerKitsImpl::GetInstance().StopStreamUpdate();
    #endif
    downloadProgress_.status = UpgradeStatus::INIT;
    sysInstallProgress_.status = UpgradeStatus::INIT;
    FIRMWARE_LOGI("DoSysInstall, status=%{public}d", component.status);
    FirmwareComponent firmwareComponent = component;

    int32_t ret = InitSysInstaller(firmwareComponent.versionId);
    if (ret != OHOS_SUCCESS) {
        return ret;
    }

    // Set the callback for the installer
    ret = SetInstallerCallback(firmwareComponent);
    if (ret != OHOS_SUCCESS) {
        return ret;
    }

    ret = StartInstallProcess(firmwareComponent);
    if (ret != OHOS_SUCCESS) {
        return ret;
    }
    return WaitInstallResult();
}

int32_t StreamInstallerInstall::InitSysInstaller(const std::string &versionId)
{
    #ifndef UPDATER_UT
    int32_t ret = SysInstaller::SysInstallerKitsImpl::GetInstance().SysInstallerInit(versionId, true);
    if (ret != OHOS_SUCCESS) {
        FIRMWARE_LOGE("sys installer init failed");
        errMsg_.errorMessage = "sys installer init failed";
        errMsg_.errorCode = DUPDATE_ERR_IPC_ERROR;
        return OHOS_FAILURE;
    }
    int32_t updateStatus = SysInstaller::SysInstallerKitsImpl::GetInstance().GetUpdateStatus(versionId);
    if (updateStatus != CAST_INT(SysInstaller::UpdateStatus::UPDATE_STATE_INIT)) {
        FIRMWARE_LOGE("StartUnpack status: %{public}d , system busy", updateStatus);
        errMsg_.errorMessage = "sys installer is busy";
        errMsg_.errorCode = ret;
        return OHOS_FAILURE;
    }
    #endif
    return OHOS_SUCCESS;
}

int32_t StreamInstallerInstall::SetInstallerCallback(FirmwareComponent &component)
{
    #ifndef UPDATER_UT
    SysInstallerExecutorCallback callback{[&](const InstallProgress &installProgress) {
        component.status = installProgress.progress.status;
        component.recordPoint += installProgress.dealLen;
        if (component.size > 0) {
            component.progress = static_cast<double>(component.recordPoint) / component.size * Firmware::ONE_HUNDRED;
        }
        if (onInstallCallback_.onFirmwareProgress == nullptr) {
            FIRMWARE_LOGE("SysInstallerExecutorCallback onFirmwareProgress is null");
            return;
        }
        onInstallCallback_.onFirmwareProgress(component);
        sysInstallProgress_ = installProgress.progress;
        if (Firmware::ONE_HUNDRED == component.progress) {
            sysInstallProgress_.status = UpgradeStatus::INSTALL_SUCCESS;
        }
        errMsg_ = installProgress.errMsg;
        if (component.status == UpgradeStatus::INSTALL_FAIL ||
            component.status == UpgradeStatus::INSTALL_SUCCESS) {
            NotifyCondition();
        }
    }};
    
    sptr<SysInstaller::ISysInstallerCallbackFunc> cb = new SysInstallerCallback(callback);
    if (cb == nullptr) {
        FIRMWARE_LOGE("sys installer callback is nullptr");
        errMsg_.errorMessage = "sys installer callback is nullptr";
        errMsg_.errorCode = DUPDATE_ERR_IPC_ERROR;
        return OHOS_FAILURE;
    }

    int32_t ret = SysInstaller::SysInstallerKitsImpl::GetInstance().SetUpdateCallback(component.versionId, cb);
    if (ret != OHOS_SUCCESS) {
        FIRMWARE_LOGE("set sys installer callback failed");
        errMsg_.errorMessage = "set sys installer callback failed";
        errMsg_.errorCode = ret;
    }
    return ret;
    #else
    return OHOS_SUCCESS;
    #endif
}

int32_t StreamInstallerInstall::StartInstallProcess(FirmwareComponent &component)
{
    #ifndef UPDATER_UT
    auto ret = SysInstaller::SysInstallerKitsImpl::GetInstance().StartStreamUpdate();
    if (ret != OHOS_SUCCESS) {
        errMsg_.errorMessage = "sys installer StartStreamUpdate failed";
        errMsg_.errorCode = ret;
        FIRMWARE_LOGE("sys installer StartStreamUpdate failed ret = %{public}d", ret);
        return OHOS_FAILURE;
    }
    downloadThread_ = std::make_shared<StreamProgressThread>([&](const Progress &progress) -> void {
        downloadProgress_ = progress;
        if (downloadProgress_.status == UpgradeStatus::DOWNLOAD_FAIL) {
            errMsg_.errorMessage = "download fail";
            errMsg_.errorCode = -1;
            NotifyCondition();
        } else if (downloadProgress_.status == UpgradeStatus::DOWNLOAD_CANCEL) {
            NotifyCondition();
        } else {
        }
    });
    ret = downloadThread_->StartDownload(component.url, component.size, component.recordPoint);
    if (ret != OHOS_SUCCESS) {
        errMsg_.errorMessage = "StartDownload failed";
        errMsg_.errorCode = ret;
        FIRMWARE_LOGE("StartDownload failed ret = %{public}d", ret);
        return OHOS_FAILURE;
    }
    #endif
    return OHOS_SUCCESS;
}

void StreamInstallerInstall::NotifyCondition()
{
    std::lock_guard<std::mutex> lock(installMutex_);
    installCond_.notify_all();
}

int32_t StreamInstallerInstall::WaitInstallResult()
{
    #ifndef UPDATER_UT
    std::unique_lock<std::mutex> lock(installMutex_);
    int32_t ret = OHOS_SUCCESS;
    uint32_t configTime = DelayedSingleton<ConfigParse>::GetInstance()->GetStreamInstallerTimeout();
    FIRMWARE_LOGI("sysinstaller wait result, max wait time=%{public}u", configTime);
    if (!installCond_.wait_for(lock, std::chrono::seconds(configTime), [this] {
        return (sysInstallProgress_.status == UpgradeStatus::INSTALL_SUCCESS) ||
        (sysInstallProgress_.status == UpgradeStatus::INSTALL_FAIL) ||
        (downloadProgress_.status == UpgradeStatus::DOWNLOAD_FAIL) ||
        (downloadProgress_.status == UpgradeStatus::DOWNLOAD_CANCEL);
    })) {
        FIRMWARE_LOGE("WaitInstallResult time out");
        ret = OHOS_FAILURE;
    } else {
        if (sysInstallProgress_.status == UpgradeStatus::INSTALL_SUCCESS) {
            FIRMWARE_LOGI("WaitInstallResult INSTALL_SUCCESS");
            ret =  OHOS_SUCCESS;
        } else {
            FIRMWARE_LOGE("WaitInstallResult fail, sysInstallProgress_.status=%{public}d, "
                "downloadProgress_.status=%{public}d", CAST_INT(sysInstallProgress_.status),
                CAST_INT(downloadProgress_.status));
            ret = OHOS_FAILURE;
        }
    }
    // 停止下载线程
    downloadThread_->StopDownload();
    // 停止数据处理线程
    SysInstaller::SysInstallerKitsImpl::GetInstance().StopStreamUpdate();
    return ret;
    #else
    return OHOS_SUCCESS;
    #endif
}
} // namespace UpdateService
} // namespace OHOS
