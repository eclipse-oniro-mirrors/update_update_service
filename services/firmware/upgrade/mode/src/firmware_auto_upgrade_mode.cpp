/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "firmware_auto_upgrade_mode.h"

#include "dupdate_net_manager.h"
#include "dupdate_upgrade_helper.h"
#include "firmware_auto_upgrade_alarm_manager.h"
#include "firmware_callback_utils.h"
#include "firmware_common.h"
#include "firmware_download_process.h"
#include "firmware_event_manager.h"
#include "firmware_status_cache.h"
#include "firmware_update_helper.h"
#include "string_utils.h"

namespace OHOS::UpdateService {

FirmwareStep FirmwareAutoUpgradeMode::GetNextStep(FirmwareStep step)
{
    FIRMWARE_LOGI("FirmwareAutoUpgradeMode::GetNextStep %{public}d", static_cast<uint32_t>(step));
    FirmwareStep nextStep = FirmwareStep::COMPLETE;
    switch (step) {
        case FirmwareStep::INIT:
            nextStep = GetStepAfterInit();
            break;
        case FirmwareStep::CHECK_STEP:
            nextStep = GetStepAfterCheck();
            break;
        case FirmwareStep::DOWNLOAD_STEP:
            nextStep = GetStepAfterDownload();
            break;
        default:
            break;
    }
    return nextStep;
}

FirmwareStep FirmwareAutoUpgradeMode::GetStepAfterInit()
{
    // 开始执行搜包，刷新下载自动搜包定时器
    FirmwareAutoUpgradeAlarmManager::GetInstance()->RefreshAutoUpgradeAlarm();
    if (!NetManager::GetInstance()->IsNetAvailable()) {
        return FirmwareStep::COMPLETE;
    }

    if (FirmwareStatusCache::GetInstance()->IsChecking()) {
        FIRMWARE_LOGI("GetStepAfterInit system busy");
        return FirmwareStep::COMPLETE;
    }

    GetTask();
    if (task_.status == UpgradeStatus::DOWNLOADING || task_.status == UpgradeStatus::UPDATING) {
        FIRMWARE_LOGI("GetStepAfterInit task is busy");
        return FirmwareStep::COMPLETE;
    }
    return FirmwareStep::CHECK_STEP;
}

FirmwareStep FirmwareAutoUpgradeMode::HandlePreDownloadStep()
{
    GetTask();
    if (!task_.isExistTask) {
        FIRMWARE_LOGI("ignored download, task is not exist");
        return FirmwareStep::COMPLETE;
    }

    UpgradeStatus taskStatus = task_.status;
    FIRMWARE_LOGI("HandlePreDownloadStep task status: %{public}d", CAST_INT(taskStatus));
    if (taskStatus == UpgradeStatus::DOWNLOAD_SUCCESS) {
        FirmwareDownloadProcess().DownloadSuccessProcess(task_, {});
        return FirmwareStep::COMPLETE;
    }

    if (taskStatus != UpgradeStatus::CHECK_VERSION_SUCCESS && taskStatus != UpgradeStatus::DOWNLOAD_FAIL &&
        taskStatus != UpgradeStatus::DOWNLOAD_PAUSE) {
        return FirmwareStep::COMPLETE;
    }
    if (taskStatus == UpgradeStatus::DOWNLOADING) {
        FIRMWARE_LOGI("HandlePreDownloadStep system busy");
        return FirmwareStep::COMPLETE;
    }

    int64_t requireTotalSize = 0;
    bool isSpaceEnough = GetDownloadDataProcessor()->IsSpaceEnough(requireTotalSize);
    if (!isSpaceEnough) {
        FIRMWARE_LOGE("HandlePreDownloadStep isSpaceEnough false");
        return FirmwareStep::COMPLETE;
    }
    return FirmwareStep::DOWNLOAD_STEP;
}

FirmwareStep FirmwareAutoUpgradeMode::GetStepAfterCheck()
{
    bool isCheckFail = checkDataProcessor_.IsCheckFailed();
    bool isSame = checkDataProcessor_.IsSameWithDb();
    bool hasNewVersion = checkDataProcessor_.HasNewVersion();
    FIRMWARE_LOGI("GetStepAfterCheck isCheckFail %{public}s isSame %{public}s hasNewVersion %{public}s",
        StringUtils::GetBoolStr(isCheckFail).c_str(), StringUtils::GetBoolStr(isSame).c_str(),
        StringUtils::GetBoolStr(hasNewVersion).c_str());
    if (isCheckFail) {
        return FirmwareStep::COMPLETE;
    }
    if (!FirmwarePreferencesUtil::GetInstance()->GetAutoDownloadSwitch()) {
        FIRMWARE_LOGI("GetStepAfterCheck, ignore download, auto download switch is off");
        return FirmwareStep::COMPLETE;
    }
    return HandlePreDownloadStep();
}

FirmwareStep FirmwareAutoUpgradeMode::GetStepAfterDownload()
{
    FirmwareTask task;
    FirmwareTaskOperator().QueryTask(task);
    FIRMWARE_LOGI("GetStepAfterDownload task status %{public}d", task.status);
    if (!task.isExistTask) {
        FIRMWARE_LOGE("GetStepAfterDownload task not exist");
        return FirmwareStep::COMPLETE;
    }
    FirmwareDownloadProcess firmwareDownloadProcess;
    ErrorMessage errorMessage;
    if (const auto &processor = GetDownloadDataProcessor(); processor != nullptr) {
        StringUtils::DecStringToNumber(processor->GetDownloadProgress().endReason, errorMessage.errorCode);
    }
    switch (task.status) {
        case UpgradeStatus::DOWNLOAD_PAUSE:
            firmwareDownloadProcess.DownloadPauseProcess(task, errorMessage);
            break;
        case UpgradeStatus::DOWNLOAD_FAIL:
            firmwareDownloadProcess.DownloadFailProcess(task, errorMessage);
            break;
        case UpgradeStatus::DOWNLOAD_CANCEL:
            firmwareDownloadProcess.DownloadCancelProcess(task, errorMessage);
            break;
        case UpgradeStatus::DOWNLOAD_SUCCESS:
            firmwareDownloadProcess.DownloadSuccessProcess(task, errorMessage);
            break;
        default:
            break;
    }
    return FirmwareStep::COMPLETE;
}

void FirmwareAutoUpgradeMode::SetCheckResult(CheckStatus status, const Duration &duration,
    const std::vector<FirmwareComponent> &componentList, const CheckAndAuthInfo &checkAndAuthInfo)
{
    FIRMWARE_LOGI("FirmwareManualCheckMode::SetCheckResult %{public}d", static_cast<uint32_t>(status));
    checkDataProcessor_.SetCheckResult(status, duration, componentList);
}

void FirmwareAutoUpgradeMode::SetDownloadCallbackInfo(const FirmwareDownloadCallbackInfo &callbackInfo)
{
    FIRMWARE_LOGI("FirmwareAutoUpgradeMode::SetDownloadCallbackInfo fake");
    const auto processor = GetDownloadDataProcessor();
    if (processor == nullptr) {
        FIRMWARE_LOGE("GetDownloadDataProcessor nullptr");
        return;
    }
    processor->SetDownloadCallbackInfo(callbackInfo);
}

void FirmwareAutoUpgradeMode::SetDownloadEvent(const std::string& downloadTaskId, EventId eventId)
{
    GetDownloadDataProcessor()->SetDownloadEvent(downloadTaskId, eventId);
}

void FirmwareAutoUpgradeMode::SetInstallResult(const InstallCallbackInfo &installCallbackInfo)
{
    installStepDataProcessor_.SetInstallResult(installCallbackInfo);
}

void FirmwareAutoUpgradeMode::HandleComplete()
{
    FIRMWARE_LOGI("FirmwareAutoUpgradeMode::HandleComplete");
    onExecuteFinishCallback_();
}

CheckMode FirmwareAutoUpgradeMode::GetCheckMode(CommonEventType autoUpgradeType)
{
    return CheckMode::AUTO;
}

CheckMode FirmwareAutoUpgradeMode::GetUpgradeCompleteCheckMode()
{
    return CheckMode::UPGRADE_COMPLETE;
}

std::shared_ptr<FirmwareDownloadDataProcessor> FirmwareAutoUpgradeMode::GetDownloadDataProcessor()
{
    if (downloadDataProcessor_ == nullptr) {
        downloadDataProcessor_ = std::make_shared<FirmwareDownloadDataProcessor>();
    }
    return downloadDataProcessor_;
}

void FirmwareAutoUpgradeMode::GetTask()
{
    FirmwareTaskOperator().QueryTask(task_);
}
} // namespace OHOS::UpdateService
