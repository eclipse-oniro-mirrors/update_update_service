/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "firmware_download_data_processor.h"

#include <algorithm>

#include "constant.h"
#include "firmware_callback_utils.h"
#include "firmware_component_operator.h"
#include "firmware_constant.h"
#include "firmware_file_utils.h"
#include "firmware_log.h"
#include "firmware_manager.h"
#include "firmware_status_cache.h"
#include "firmware_task_operator.h"
#include "string_utils.h"

namespace OHOS {
namespace UpdateService {
FirmwareDownloadDataProcessor::FirmwareDownloadDataProcessor()
{
    GetTask();
    downloadProgress_.percent = static_cast<uint32_t>(tasks_.progress);
    downloadProgress_.status = tasks_.status;
    FIRMWARE_LOGI("FirmwareDownloadDataProcessor::FirmwareDownloadDataProcessor, "
        "status: %{public}d progress: %{public}d",
        downloadProgress_.status, downloadProgress_.percent);
}

FirmwareDownloadDataProcessor::~FirmwareDownloadDataProcessor()
{
    FIRMWARE_LOGD("FirmwareDownloadDataProcessor::~FirmwareDownloadDataProcessor");
}

bool FirmwareDownloadDataProcessor::IsDownloadSuccess()
{
    FirmwareTask task;
    FirmwareTaskOperator().QueryTask(task);
    if (!task.isExistTask) {
        FIRMWARE_LOGI("IsDownloadSuccess no task");
        return false;
    }
    FIRMWARE_LOGI("IsDownloadSuccess task status %{public}d", task.status);
    return task.status == UpgradeStatus::DOWNLOAD_SUCCESS;
}

bool FirmwareDownloadDataProcessor::IsSpaceEnough(int64_t &requireTotalSize)
{
    std::vector<FirmwareComponent> components;
    FirmwareComponentOperator().QueryAll(components);
    for (const FirmwareComponent &component : components) {
        requireTotalSize += (Firmware::ONE_HUNDRED - component.progress) * component.size / Firmware::ONE_HUNDRED;
    }
    bool isDownloadSpaceEnough = FirmwareFileUtils::IsSpaceEnough(requireTotalSize);
    FIRMWARE_LOGI("IsSpaceEnough %{public}s", StringUtils::GetBoolStr(isDownloadSpaceEnough).c_str());
    return isDownloadSpaceEnough;
}

void FirmwareDownloadDataProcessor::SetDownloadCallbackInfo(const FirmwareDownloadCallbackInfo &callbackInfo)
{
    std::string taskId;
    Progress progress;
    {
        std::lock_guard<std::recursive_mutex> guard(GetFirmwareCancelMutex());
        GetTask();
        if (!tasks_.isExistTask || tasks_.status == UpgradeStatus::CHECK_VERSION_SUCCESS) {
            FIRMWARE_LOGI("SetDownloadCallbackInfo discard, task canceled or not exists, status: %{public}d",
                CAST_INT(tasks_.status));
            return;
        }
        uint32_t recordedProgress = downloadProgress_.percent;
        downloadProgress_ = callbackInfo.taskProgress;
        downloadProgress_.percent = std::max(downloadProgress_.percent, recordedProgress);
        FIRMWARE_LOGI("SetDownloadProgress status: %{public}d progress: %{public}d", downloadProgress_.status,
            downloadProgress_.percent);

        for (const auto &componentProgress : callbackInfo.progressList) {
            FIRMWARE_LOGI("component versionId: %{public}s  status: %{public}d progress is: %{public}d",
                componentProgress.versionId.c_str(), CAST_INT(componentProgress.progress.status),
                CAST_INT(componentProgress.progress.percent));
            componentOperator_.UpdateProgressByVersionId(componentProgress.versionId,
                componentProgress.progress.status, CAST_INT(componentProgress.progress.percent));
        }
        taskOperator_.UpdateProgressByTaskId(tasks_.taskId, downloadProgress_.status,
            CAST_INT(downloadProgress_.percent));
        taskId = tasks_.taskId;
        progress = downloadProgress_;
    }
    FirmwareCallbackUtils::GetInstance()->ProgressCallback(taskId, progress);
}

void FirmwareDownloadDataProcessor::SetDownloadEvent(const std::string &downloadTaskId, EventId eventId)
{
    std::string taskId;
    int32_t progress = 0;
    {
        std::lock_guard<std::recursive_mutex> guard(GetFirmwareCancelMutex());
        GetTask();
        if (!tasks_.isExistTask) {
            FIRMWARE_LOGI("SetDownloadEvent discard, status: %{public}d", CAST_INT(tasks_.status));
            return;
        }
        taskId = tasks_.taskId;
        if (eventId == EventId::EVENT_DOWNLOAD_START) {
            FIRMWARE_LOGI("SetDownloadEvent handle download start");
            taskOperator_.UpdateDownloadTaskIdByTaskId(tasks_.taskId, downloadTaskId);
            taskOperator_.UpdateStatusByTaskId(tasks_.taskId, UpgradeStatus::DOWNLOADING);
        } else if (eventId == EventId::EVENT_DOWNLOAD_RESUME) {
            FIRMWARE_LOGI("SetDownloadEvent handle download resume");
            taskOperator_.UpdateStatusByTaskId(tasks_.taskId, UpgradeStatus::DOWNLOADING);
            progress = tasks_.progress;
        } else {
            return;
        }
    }
    if (eventId == EventId::EVENT_DOWNLOAD_START) {
        FirmwareCallbackUtils::GetInstance()->NotifyEvent(taskId, EventId::EVENT_DOWNLOAD_START,
            UpgradeStatus::DOWNLOADING);
    }

    if (eventId == EventId::EVENT_DOWNLOAD_RESUME) {
        FirmwareCallbackUtils::GetInstance()->NotifyEvent(taskId, EventId::EVENT_DOWNLOAD_RESUME,
            UpgradeStatus::DOWNLOADING, {}, {}, progress);
    }
}

void FirmwareDownloadDataProcessor::GetTask()
{
    FirmwareTaskOperator().QueryTask(tasks_);
}

Progress FirmwareDownloadDataProcessor::GetDownloadProgress()
{
    return downloadProgress_;
}
} // namespace UpdateService
} // namespace OHOS
