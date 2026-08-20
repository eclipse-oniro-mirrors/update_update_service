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

#include "firmware_stream_install_apply_mode.h"

#include <utility>

#include "dupdate_errno.h"
#include "dupdate_net_manager.h"
#include "dupdate_upgrade_helper.h"
#include "firmware_callback_utils.h"
#include "firmware_changelog_utils.h"
#include "firmware_common.h"
#include "firmware_component_operator.h"
#include "firmware_constant.h"
#include "firmware_log.h"
#include "firmware_preferences_utils.h"
#include "firmware_status_cache.h"
#include "firmware_task_operator.h"
#include "firmware_update_helper.h"

#include "battery_state_utils.h"
#include "device_idle_utils.h"

namespace OHOS {
namespace UpdateService {
FirmwareStep FirmwareStreamInstallApplyMode::GetNextStep(FirmwareStep step)
{
    FIRMWARE_LOGI("GetNextStep %{public}d", static_cast<uint32_t>(step));
    FirmwareStep nextStep = FirmwareStep::COMPLETE;
    switch (step) {
        case FirmwareStep::INIT:
            nextStep = GetStepAfterInit();
            break;
        case FirmwareStep::INSTALL_STEP:
            nextStep = GetStepAfterInstall();
            break;
        case FirmwareStep::APPLY_STEP:
            nextStep = GetStepAfterApply();
            break;
        default:
            break;
    }
    return nextStep;
}

FirmwareStep FirmwareStreamInstallApplyMode::GetStepAfterInit()
{
    GetTask();
    if (!tasks_.isExistTask) {
        FIRMWARE_LOGI("GetStepAfterInit no task");
        businessError_.Build(CallResult::FAIL, "no task!");
        return FirmwareStep::COMPLETE;
    }

    FIRMWARE_LOGI("GetStepAfterInit status %{public}d taskId = %{public}s, order = %{public}d",
        static_cast<int32_t>(tasks_.status), tasks_.taskId.c_str(), CAST_INT(upgradeOptions_.order));

    if (upgradeOptions_.order == Order::INSTALL || upgradeOptions_.order == Order::INSTALL_AND_APPLY) {
        return GetStepForInstallAndApplyOrder();
    }

    if (upgradeOptions_.order == Order::APPLY) {
        return FirmwareStep::APPLY_STEP;
    }

    businessError_.Build(CallResult::FAIL, "upgradeOptions error");
    return FirmwareStep::COMPLETE;
}

FirmwareStep FirmwareStreamInstallApplyMode::GetStepForInstallAndApplyOrder()
{
    if (!IsAllowInstall()) {
        return FirmwareStep::COMPLETE;
    }

    return FirmwareStep::INSTALL_STEP;
}

bool FirmwareStreamInstallApplyMode::IsAllowInstall()
{
    FIRMWARE_LOGI("IsAllowInstall is StreamUpgrade");
    if (installType_ != InstallType::STREAM_INSTALLLER) {
        FIRMWARE_LOGE("IsAllowInstall stream is not support");
        businessError_.Build(CallResult::FAIL, "install condition fail!");
        businessError_.AddErrorMessage(CAST_INT(DUPDATE_ERR_INPUT_PARA_ERROR), "install type error");
        return false;
    }
    if (!DelayedSingleton<NetManager>::GetInstance()->IsNetAvailable()) {
        FIRMWARE_LOGI("IsAllowInstall IsNeedBlockCheck no network, nettype is %{public}d",
            DelayedSingleton<NetManager>::GetInstance()->GetNetType());
        businessError_.Build(CallResult::FAIL, "download no network!");
        businessError_.AddErrorMessage(CAST_INT(DUPDATE_ERR_DLOAD_REQUEST_FAIL), "download no network!");
        return false;
    }
    if (!BatteryStateUtils::IsBatteryEnough(MANUAL_UPGRADE_BATTERY_LEVEL)) {
        FIRMWARE_LOGE("IsAllowInstall isBatteryEnough is false");
        businessError_.Build(CallResult::FAIL, "install condition fail!");
        businessError_.AddErrorMessage(CAST_INT(DUPDATE_ERR_LOW_BATTERY_LEVEL), "battery is low");
        return false;
    }
    if (tasks_.upgradeMode == UpgradeMode::NIGHT) {
        return DeviceIdleUtils::IsDeviceIdle();
    }
    return true;
}

FirmwareStep FirmwareStreamInstallApplyMode::GetStepAfterInstall()
{
    FIRMWARE_LOGI("FirmwareStreamInstallApplyMode installOptions %{public}d", CAST_INT(upgradeOptions_.order));
    if (installStepDataProcessor_.HasInstallFail()) {
        FirmwareUpdateHelper::ClearFirmwareInfo();
    }

    if (installStepDataProcessor_.HasInstallSuccess() && upgradeOptions_.order == Order::INSTALL_AND_APPLY) {
        return FirmwareStep::APPLY_STEP;
    }

    return FirmwareStep::COMPLETE;
}

FirmwareStep FirmwareStreamInstallApplyMode::GetStepAfterApply()
{
    return FirmwareStep::COMPLETE;
}

void FirmwareStreamInstallApplyMode::GetTask()
{
    if (!tasks_.isExistTask) {
        FirmwareTaskOperator().QueryTask(tasks_);
        FIRMWARE_LOGI("FirmwareStreamInstallApplyMode::GetTask status: %{public}d ", CAST_INT(tasks_.status));
    }
}

void FirmwareStreamInstallApplyMode::HandleComplete()
{
    onExecuteFinishCallback_();
}

UpgradeOptions FirmwareStreamInstallApplyMode::GetUpgradeOptions()
{
    return upgradeOptions_;
}

InstallType FirmwareStreamInstallApplyMode::GetInstallType()
{
    return installType_;
}

void FirmwareStreamInstallApplyMode::SetInstallResult(const InstallCallbackInfo &installCallbackInfo)
{
    installStepDataProcessor_.SetInstallResult(installCallbackInfo);
}

void FirmwareStreamInstallApplyMode::SetApplyResult(bool isSuccess)
{
    FIRMWARE_LOGI("SetApplyResult isSuccess: %{public}s", isSuccess ? "success" : "fail");
}
} // namespace UpdateService
} // namespace OHOS