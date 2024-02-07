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

#include "config_parse.h"
#include "dupdate_errno.h"
#include "firmware_constant.h"
#include "firmware_log.h"
#include "firmware_sys_installer_callback.h"
#include "firmware_update_helper.h"

namespace OHOS {
namespace UpdateEngine {

bool SysInstallerInstall::IsComponentLegal(const std::vector<FirmwareComponent> &componentList)
{
    return FirmwareUpdateHelper::IsUpgradePackagesReady(componentList);
}

bool SysInstallerInstall::PerformInstall(const std::vector<FirmwareComponent> &componentList)
{
    FIRMWARE_LOGI("SysInstallerInstall::PerformInstall");
    if (componentList.empty()) {
        return false;
    }
    uint32_t successCount = 0;
    for (const auto &component : componentList) {
        onInstallCallback_.onFirmwareStatus(UpgradeStatus::INSTALLING);
        if (DoSysInstall(component) == OHOS_SUCCESS) {
            successCount ++;
        }
    }
    return successCount == static_cast<uint32_t>(componentList.size());
}

int32_t SysInstallerInstall::DoSysInstall(const FirmwareComponent &firmwareComponent)
{
    FIRMWARE_LOGI("DoSysInstall, status=%{public}d", firmwareComponent.status);
    FirmwareComponent sysComponent = firmwareComponent;
    InitInstallProgress();
    int32_t ret = SysInstaller::SysInstallerKitsImpl::GetInstance().SysInstallerInit();
    if (ret != OHOS_SUCCESS) {
        FIRMWARE_LOGE("sys installer init failed");
        errMsg_.errorMessage = "sys installer init failed";
        errMsg_.errorCode = DUPDATE_ERR_IPC_ERROR;
        return OHOS_FAILURE;
    }

    int32_t updateStatus = SysInstaller::SysInstallerKitsImpl::GetInstance().GetUpdateStatus();
    if (updateStatus != CAST_INT(SysInstaller::UpdateStatus::UPDATE_STATE_INIT)) {
        FIRMWARE_LOGE("StartUnpack status: %{public}d , system busy", updateStatus);
        errMsg_.errorMessage = "sys installer is busy";
        errMsg_.errorCode = ret;
        return OHOS_FAILURE;
    }

    SysInstallerExecutorCallback callback { [&](const InstallProgress &installProgress) {
        sysInstallProgress_ = installProgress.progress;
        errMsg_ = installProgress.errMsg;
        sysComponent.status = installProgress.progress.status;
        sysComponent.progress = installProgress.progress.percent;
        FIRMWARE_LOGI("SysInstallerExecutorCallback status=%{public}d , progress=%{public}d",
            sysComponent.status, sysComponent.progress);
        onInstallCallback_.onFirmwareProgress(sysComponent);
    } };
    sptr<SysInstaller::ISysInstallerCallbackFunc> cb = new SysInstallerCallback(callback);
    if (cb == nullptr) {
        FIRMWARE_LOGE("sys installer callback is nullptr");
        errMsg_.errorMessage = "sys installer callback is nullptr";
        errMsg_.errorCode = DUPDATE_ERR_IPC_ERROR;
        return OHOS_FAILURE;
    }

    ret = SysInstaller::SysInstallerKitsImpl::GetInstance().SetUpdateCallback(cb);
    if (ret != OHOS_SUCCESS) {
        FIRMWARE_LOGE("set sys installer callback failed");
        errMsg_.errorMessage = "set sys installer callback failed";
        errMsg_.errorCode = ret;
        return OHOS_FAILURE;
    }

    ret = SysInstaller::SysInstallerKitsImpl::GetInstance().StartUpdatePackageZip(sysComponent.spath);
    if (ret != OHOS_SUCCESS) {
        errMsg_.errorMessage = "sys installer StartUpdatePackageZip failed";
        errMsg_.errorCode = ret;
        FIRMWARE_LOGE("sys installer StartUpdatePackageZip failed ret = %{public}d", ret);
        return OHOS_FAILURE;
    }
    return WaitInstallResult();
}

void SysInstallerInstall::InitInstallProgress()
{
    sysInstallProgress_.status = UpgradeStatus::INSTALLING;
    sysInstallProgress_.percent = 0;
    sysInstallProgress_.endReason = "";
    errMsg_.errorCode = 0;
    errMsg_.errorMessage = "";
}

int32_t SysInstallerInstall::WaitInstallResult()
{
    uint32_t timeout = 0;
    uint32_t configTime = DelayedSingleton<ConfigParse>::GetInstance()->GetAbInstallerTimeout();
    FIRMWARE_LOGI("sysinstaller wait result, max wait time=%{public}u", configTime);
    while (timeout <= configTime) {
        if (sysInstallProgress_.status == UpgradeStatus::INSTALL_FAIL) {
            FIRMWARE_LOGE("WaitInstallResult sysinstaller fail");
            return OHOS_FAILURE;
        }
        if (sysInstallProgress_.status == UpgradeStatus::INSTALL_SUCCESS &&
            sysInstallProgress_.percent == Firmware::ONE_HUNDRED) {
            return OHOS_SUCCESS;
        }
        timeout++;
        sleep(SLEEP_INSTALL);
    }
    FIRMWARE_LOGI("WaitInstallResult time out, sysInstallProgress_.status=%{public}d",
        CAST_INT(sysInstallProgress_.status));
    return OHOS_FAILURE;
}
} // namespace UpdateEngine
} // namespace OHOS
