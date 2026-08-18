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

#ifndef DOWNLOAD_CALLBACK_H
#define DOWNLOAD_CALLBACK_H

#include <string>
#include <vector>

#include "download_common.h"
#include "update_define.h"

namespace OHOS::UpdateService {
struct DownloadProgress {
    std::string id;
    uint32_t percent = 0;
    int64_t startTime = 0;
    int64_t endTime = 0;
    int64_t effectiveTime = 0;
    DownloadStatus status = DownloadStatus::INIT;
    DownloadEndReason reason = DownloadEndReason::INIT;
    DownloadErrorInfo downloadErrorInfo;
    std::string downloadUrl;
    int64_t packageSize = 0;
    int64_t downloadedSize = 0;
    int64_t verifiedSize = 0;
    bool isCombineVerifyProgress = false;

    explicit DownloadProgress(const DownloadInfo &info)
    {
        id = info.versionId;
        percent =
            (info.packageSize == 0) ? 0 : (uint32_t)(info.downloadedSize * DOWNLOAD_MAX_PERCENT / info.packageSize);
        status = info.status;
        reason = info.endReason;
        startTime = info.startTime;
        endTime = info.endTime;
        effectiveTime = info.effectiveTime;
        downloadErrorInfo = info.downloadErrorInfo;
        downloadUrl = info.url;
        packageSize = info.packageSize;
        downloadedSize = info.downloadedSize;
        verifiedSize = info.verifiedSize;
        isCombineVerifyProgress = info.isCombineVerifyProgress;
    }
};

struct ProgressInfo {
    int32_t taskProgress = 0;
    DownloadStatus taskStatus = DownloadStatus::INIT;
    DownloadEndReason reason = DownloadEndReason::INIT;
    std::vector<DownloadProgress> progresses;
    int64_t packageSize = 0;
    int64_t downloadedSize = 0;
    int64_t verifiedSize = 0;

    [[nodiscard]] bool IsDownloadFinished() const
    {
        return taskStatus == DownloadStatus::SUCCESS || taskStatus == DownloadStatus::FAIL;
    }

    [[nodiscard]] bool IsTaskCompleted() const
    {
        return taskStatus == DownloadStatus::SUCCESS || taskStatus == DownloadStatus::FAIL ||
            taskStatus == DownloadStatus::CANCEL || taskStatus == DownloadStatus::AUTO_PAUSE ||
            taskStatus == DownloadStatus::PAUSE;
    }

    [[nodiscard]] bool IsSyncCallback() const
    {
        return taskStatus == DownloadStatus::CANCEL || taskStatus == DownloadStatus::PAUSE;
    }

    [[nodiscard]] std::string ToString() const
    {
        std::string str("ProgressInfo[");
        str.append("taskProgress=").append(std::to_string(taskProgress)).append(",");
        str.append("taskStatus=").append(std::to_string(CAST_INT(taskStatus))).append(",");
        str.append("packageSize=").append(std::to_string(packageSize)).append(",");
        str.append("downloadedSize=").append(std::to_string(downloadedSize)).append(",");
        str.append("verifiedSize=").append(std::to_string(verifiedSize)).append(",");
        str.append("reason=").append(std::to_string(CAST_INT(reason))).append("]");
        return str;
    }
};

using DownloadCallback = std::function<void(const std::string &taskId, ProgressInfo progressInfo)>;
} // namespace OHOS::UpdateService
#endif // DOWNLOAD_CALLBACK_H