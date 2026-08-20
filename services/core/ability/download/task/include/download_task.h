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

#ifndef DOWNLOAD_TASK_H
#define DOWNLOAD_TASK_H

#include <atomic>
#include <map>
#include <string>

#include "download_callback.h"
#include "download_service.h"
#include "download_task_progress_processor.h"

namespace OHOS::UpdateService {
class DownloadTask : public std::enable_shared_from_this<DownloadTask> {
public:
    DownloadTask(const std::string &taskId, std::vector<DownloadInfo> &downloadInfos);
    ~DownloadTask();

    std::string Start(DownloadCallback callback, DownloadError &downloadError);
    void Pause(DownloadError &downloadError);
    void Resume(NetType netType, DownloadCallback callback, DownloadError &downloadError);
    void Cancel(DownloadError &downloadError, bool isDeleteFiles = true);
    void Recover();

    ProgressInfo GetTaskProgress();
    void OnNetChange(NetType netType);
    void DestroyTask();

private:
    void InitDownloadInfo(std::vector<DownloadInfo> &downloadInfos);
    bool IsMemoryEnough(int64_t size);

    void UpdateTaskProgress(const DownloadCallbackInfo &info);

    std::string GetRootPath() const;
    std::string GetTaskId() const;
    NetType GetRequiredNetType() const;
    void SubmitTimedTask();
    void CancelTimedTask();

    void DisableCallback();
    void EnableCallback();
    void NotifyCallbackDisabled(bool flag = true);

    void CallbackProgress(const ProgressInfo &progressInfo);
    void CallbackProgressLocked(const ProgressInfo &progressInfo);
    bool IsProgressChanged(const ProgressInfo &progressInfo);
    uint64_t CalcNextReportInterval() const;
    bool IsConcurrentLimited(std::vector<DownloadInfo> &downloadInfos);

private:
    ffrt::recursive_mutex callbackMutex_;
    DownloadStatus status_ = DownloadStatus::INIT;

    int64_t reportedTaskProgress_ = 0;
    std::string taskId_;
    std::string rootPath_;

    std::map<std::string, std::shared_ptr<DownloadInfo>> downloadInfoMap_;
    DownloadCallback callback_ = nullptr;
    std::unique_ptr<DownloadService> downloadService_ = nullptr;

    // 定时上报任务队列
    ffrt::queue timedReportTaskQueue_{ "timed_report_task_queue", ffrt::queue_attr().qos(ffrt::qos_utility) };
    ffrt::task_handle timedTaskHandle_;
    int64_t lastReportTime_ = 0;

    // 状态处理
    std::unique_ptr<DownloadTaskProgressProcessor> taskProgressProcessor_ = nullptr;
    ProgressInfo progressInfo_;
    ffrt::recursive_mutex downloadProgressUpdateMutex_;
    std::atomic<bool> isStopDownloadingCallback_ = false;
};
} // namespace OHOS::UpdateService
#endif // DOWNLOAD_TASK_H