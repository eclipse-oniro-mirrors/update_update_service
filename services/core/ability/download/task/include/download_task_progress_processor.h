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

#ifndef DOWNLOAD_TASK_PROGRESS_PROCESSOR_H
#define DOWNLOAD_TASK_PROGRESS_PROCESSOR_H

#include <memory>
#include <mutex>
#include <vector>

#include "download_callback.h"
#include "download_common.h"

namespace OHOS::UpdateService {
class DownloadTaskProgressProcessor {
public:
    explicit DownloadTaskProgressProcessor(const std::vector<std::shared_ptr<DownloadInfo>> &downloadInfos);
    ~DownloadTaskProgressProcessor();

    ProgressInfo GetTaskProgress();

private:
    void Update();
    [[nodiscard]] int32_t CalcDownloadProgress() const;
    [[nodiscard]] DownloadStatus CalcDownloadTaskStatus(const std::map<DownloadStatus, int32_t> &countMap) const;
    [[nodiscard]] int32_t GetStatusCount(const std::map<DownloadStatus, int32_t> &countMap,
        DownloadStatus status) const;

private:
    // 初始化之后不变的参数
    int32_t totalInfoSize_ = 0;
    int64_t packageSize_ = 0;
    std::vector<std::shared_ptr<DownloadInfo>> downloadInfos_;

    // 初始化后会变更的参数
    int64_t downloadedSize_ = 0;
    int64_t verifiedSize_ = 0;
    [[maybe_unused]] int64_t taskProgress_ = 0;
    DownloadStatus status_ = DownloadStatus::INIT;
    DownloadEndReason endReason_ = DownloadEndReason::INIT;

    std::mutex updateMutex_;
    std::string taskId_;
};
} // namespace OHOS::UpdateService
#endif // DOWNLOAD_TASK_PROGRESS_PROCESSOR_H
