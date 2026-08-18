/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "firmware_sys_installer_install.h"

#include <dirent.h>
#include <iostream>
#include <unistd.h>

#include "isys_installer.h"
#include "sys_installer_kits_impl.h"
#include "sys_installer_task_const.h"

#include "config_parse.h"
#include "dupdate_errno.h"
#include "firmware_constant.h"
#include "firmware_log.h"
#include "firmware_sys_installer_callback.h"
#include "firmware_update_helper.h"

namespace OHOS {
namespace UpdateService {

bool SysInstallerInstall::IsComponentLegal(const std::vector<FirmwareComponent> &componentList)
{
    return FirmwareUpdateHelper::IsUpgradePackagesReady(componentList);
}

bool SysInstallerInstall::PerformInstall(const std::vector<FirmwareComponent> &componentList, UpgradeStatus &status)
{
    FIRMWARE_LOGI("SysInstallerInstall::PerformInstall");
    if (componentList.empty()) {
        return false;
    }
    uint32_t successCount = 0;
    for (const auto &component : componentList) {
        if (onInstallCallback_.onFirmwareStatus == nullptr) {
            FIRMWARE_LOGE("SysInstallerInstall PerformInstall onFirmwareStatus is null");
            continue;
        }
        onInstallCallback_.onFirmwareStatus(UpgradeStatus::INSTALLING);
        if (DoSysInstall(component) == OHOS_SUCCESS) {
            successCount ++;
        }
    }
    return successCount == static_cast<uint32_t>(componentList.size());
}

int32_t SysInstallerInstall::InitSysInstaller(const FirmwareComponent &firmwareComponent)
{
    int32_t ret = SysInstaller::SysInstallerKitsImpl::GetInstance().SysInstallerInit(firmwareComponent.versionId);
    if (ret != OHOS_SUCCESS) {
        FIRMWARE_LOGE("sys installer init failed");
        errMsg_.errorMessage = "sys installer init failed";
        errMsg_.errorCode = DUPDATE_ERR_IPC_ERROR;
        return OHOS_FAILURE;
    }
    return OHOS_SUCCESS;
}

int32_t SysInstallerInstall::SetupInstallCallback(FirmwareComponent firmwareComponent)
{
    SysInstallerExecutorCallback callback { [this, firmwareComponent](const InstallProgress &installProgress) mutable {
        {
            std::lock_guard<std::mutex> lock(installMutex_);
            sysInstallProgress_ = installProgress.progress;
            errMsg_ = installProgress.errMsg;
            resultReady_.store(true);
        }

        installCond_.notify_one();
        firmwareComponent.status = installProgress.progress.status;
        firmwareComponent.progress = installProgress.progress.percent;
        FIRMWARE_LOGI("SysInstallerExecutorCallback status=%{public}d , progress=%{public}d",
            firmwareComponent.status, firmwareComponent.progress);
        if (onInstallCallback_.onFirmwareProgress == nullptr) {
            FIRMWARE_LOGE("SysInstallerExecutorCallback onFirmwareProgress is null");
            return;
        }
        onInstallCallback_.onFirmwareProgress(firmwareComponent);
    } };
    sptr<SysInstaller::ISysInstallerCallbackFunc> cb = new SysInstallerCallback(callback);
    if (cb == nullptr) {
        FIRMWARE_LOGE("sys installer callback is nullptr");
        errMsg_.errorMessage = "sys installer callback is nullptr";
        errMsg_.errorCode = DUPDATE_ERR_IPC_ERROR;
        return OHOS_FAILURE;
    }

    int32_t ret = SysInstaller::SysInstallerKitsImpl::GetInstance().SetUpdateCallback(firmwareComponent.versionId, cb);
    if (ret != OHOS_SUCCESS) {
        FIRMWARE_LOGE("set sys installer callback failed");
        errMsg_.errorMessage = "set sys installer callback failed";
        errMsg_.errorCode = ret;
        return OHOS_FAILURE;
    }
    return OHOS_SUCCESS;
}

int32_t SysInstallerInstall::DoSysInstall(const FirmwareComponent &firmwareComponent)
{
    FIRMWARE_LOGI("DoSysInstall, status=%{public}d", firmwareComponent.status);
    FirmwareComponent sysComponent = firmwareComponent;
    InitInstallProgress();

    if (InitSysInstaller(sysComponent) != OHOS_SUCCESS) {
        FIRMWARE_LOGE("InitSysInstaller error");
        return OHOS_FAILURE;
    }

    int32_t updateStatus = SysInstaller::SysInstallerKitsImpl::GetInstance().GetUpdateStatus(sysComponent.versionId);
    if (updateStatus != CAST_INT(SysInstaller::UpdateStatus::UPDATE_STATE_INIT)) {
        FIRMWARE_LOGE("StartUnpack status: %{public}d , system busy", updateStatus);
        errMsg_.errorMessage = "sys installer is busy";
        errMsg_.errorCode = DUPDATE_ERR_SYSTEM_BUSY_ON_INSTALL;
        return OHOS_FAILURE;
    }

    if (SetupInstallCallback(sysComponent) != OHOS_SUCCESS) {
        FIRMWARE_LOGE("install callback setup error");
        return OHOS_FAILURE;
    }

    if (StartUpdatePackageZip(sysComponent.versionId, sysComponent.spath) != OHOS_SUCCESS) {
        FIRMWARE_LOGE("startUpdatePackageZip error");
        return OHOS_FAILURE;
    }
    return WaitInstallResult(sysComponent.versionId);
}

int32_t SysInstallerInstall::StartUpdatePackageZip(const std::string &versionId, std::string &path)
{
    auto ret = SysInstaller::SysInstallerKitsImpl::GetInstance().StartUpdatePackageZip(versionId, path);
    if (ret != OHOS_SUCCESS) {
        errMsg_.errorMessage = "sys installer StartUpdatePackageZip failed";
        errMsg_.errorCode = ret;
        FIRMWARE_LOGE("sys installer StartUpdatePackageZip failed ret = %{public}d", ret);
        return OHOS_FAILURE;
    }
    return ret;
}

void SysInstallerInstall::InitInstallProgress()
{
    sysInstallProgress_.status = UpgradeStatus::INSTALLING;
    sysInstallProgress_.percent = 0;
    sysInstallProgress_.endReason = "";
    errMsg_.errorCode = 0;
    errMsg_.errorMessage = "";
    resultReady_.store(false);
}

int32_t SysInstallerInstall::WaitInstallResult(const std::string &versionId)
{
    std::unique_lock<std::mutex> lock(installMutex_);
    uint32_t configTime = DelayedSingleton<ConfigParse>::GetInstance()->GetAbInstallerTimeout();
    FIRMWARE_LOGI("sysinstaller wait result, max wait time=%{public}u", configTime);

    if (!installCond_.wait_for(lock, std::chrono::seconds(configTime), [this] {
            return resultReady_.load() && ((sysInstallProgress_.status == UpgradeStatus::INSTALL_SUCCESS &&
                sysInstallProgress_.percent == Firmware::ONE_HUNDRED) ||
                (sysInstallProgress_.status == UpgradeStatus::INSTALL_FAIL));
        })) {
        FIRMWARE_LOGI("WaitInstallResult time out, sysInstallProgress_.status = %{public}d",
            CAST_INT(sysInstallProgress_.status));
        return OHOS_FAILURE;
    }
    
    bool isSuccess = (sysInstallProgress_.status == UpgradeStatus::INSTALL_SUCCESS &&
                      sysInstallProgress_.percent == Firmware::ONE_HUNDRED);

    if (isSuccess) {
        FIRMWARE_LOGI("WaitInstallResult INSTALL_SUCCESS");
    } else {
        FIRMWARE_LOGE("WaitInstallResult fail, sysInstallProgress_.status=%{public}d",
            CAST_INT(sysInstallProgress_.status));
    }

    SysInstaller::SysInstallerKitsImpl::GetInstance().GetUpdateResult(versionId,
        SysInstaller::TaskTypeConst::TASK_TYPE_AB_UPDATE,
        SysInstaller::ResultTypeConst::RESULT_TYPE_INSTALL);
    return isSuccess ? OHOS_SUCCESS : OHOS_FAILURE;
}
} // namespace UpdateService
} // namespace OHOS
