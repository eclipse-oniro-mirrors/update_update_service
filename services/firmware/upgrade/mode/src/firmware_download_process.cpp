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

#include "firmware_download_process.h"

#include "download_task_manager.h"
#include "firmware_callback_utils.h"
#include "firmware_file_utils.h"
#include "firmware_log.h"
#include "firmware_night_upgrade_alarm_manager.h"
#include "firmware_update_helper.h"

namespace OHOS::UpdateService {
void FirmwareDownloadProcess::DownloadPauseProcess(const FirmwareTask &task, const ErrorMessage &errorMessage)
{
    FIRMWARE_LOGI("DownloadPauseProcess download pause");
    DelayedSingleton<FirmwareCallbackUtils>::GetInstance()->NotifyEvent(task.taskId, EventId::EVENT_DOWNLOAD_PAUSE,
        UpgradeStatus::DOWNLOAD_PAUSE, errorMessage);
}

void FirmwareDownloadProcess::DownloadFailProcess(const FirmwareTask &task, const ErrorMessage &errorMessage)
{
    FIRMWARE_LOGI("DownloadFailProcess download fail");
    FirmwareCallbackUtils::GetInstance()->NotifyEvent(task.taskId, EventId::EVENT_DOWNLOAD_FAIL,
        UpgradeStatus::DOWNLOAD_FAIL, errorMessage);
    FirmwareUpdateHelper::ClearFirmwareInfo();
}

void FirmwareDownloadProcess::DownloadCancelProcess(const FirmwareTask &task, const ErrorMessage &errorMessage,
    bool isDeleteFiles)
{
    FIRMWARE_LOGI("DownloadCancelProcess, isDeleteFiles = %{public}d", isDeleteFiles);
    DelayedSingleton<FirmwareCallbackUtils>::GetInstance()->NotifyEvent(task.taskId, EventId::EVENT_DOWNLOAD_CANCEL,
        UpgradeStatus::DOWNLOAD_CANCEL, errorMessage);
    FirmwareTaskOperator().UpdateProgressByTaskId(task.taskId, UpgradeStatus::CHECK_VERSION_SUCCESS, 0);
    std::vector<FirmwareComponent> firmwareComponentList;
    FirmwareComponentOperator firmwareComponentOperator;
    firmwareComponentOperator.QueryAll(firmwareComponentList);
    for (const FirmwareComponent &component : firmwareComponentList) {
        firmwareComponentOperator.UpdateProgressByUrl(component.url, UpgradeStatus::CHECK_VERSION_SUCCESS, 0);
    }
    DownloadError error;
    DownloadTaskManager::GetInstance()->RemoveTask(task.downloadTaskId, error);

    FirmwareTaskOperator().UpdateDownloadTaskIdByTaskId(task.taskId, "");
    FirmwareFileUtils::DeleteDownloadFiles();
}

void FirmwareDownloadProcess::DownloadSuccessProcess(const FirmwareTask &task, const ErrorMessage &errorMessage)
{
    FIRMWARE_LOGI("GetStepAfterDownload download success");
    DelayedSingleton<FirmwareCallbackUtils>::GetInstance()->NotifyEvent(task.taskId, EventId::EVENT_UPGRADE_WAIT,
        UpgradeStatus::DOWNLOAD_SUCCESS);

    if (FirmwarePreferencesUtil::GetInstance()->GetNightUpgradeSwitch()) {
        FIRMWARE_LOGI("DownloadSuccessProcess NightUpgradeSwitch is on, SetNightUpgradeAlarm");
        FirmwareNightUpgradeAlarmManager::GetInstance()->SetNightUpgradeAlarm();
    }
    DownloadError error;
    DownloadTaskManager::GetInstance()->RemoveTask(task.downloadTaskId, error);
}
} // namespace OHOS::UpdateEngine
