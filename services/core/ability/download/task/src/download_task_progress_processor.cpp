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

#include "download_task_progress_processor.h"

#include "update_log.h"

namespace OHOS::UpdateService {
DownloadTaskProgressProcessor::DownloadTaskProgressProcessor(
    const std::vector<std::shared_ptr<DownloadInfo>> &downloadInfos)
{
    downloadInfos_ = downloadInfos;

    // 计算总大小
    for (const auto &downloadInfo : downloadInfos) {
        if (downloadInfo == nullptr) {
            continue;
        }
        totalInfoSize_++;
        packageSize_ += downloadInfo->packageSize;
        downloadedSize_ += downloadInfo->downloadedSize;
        taskId_ = downloadInfo->taskId;
    }
    ENGINE_LOGI("DownloadTaskProgressProcessor, taskId %{public}s", taskId_.c_str());
}

DownloadTaskProgressProcessor::~DownloadTaskProgressProcessor()
{
    ENGINE_LOGI("DownloadTaskProgressProcessor destructor, taskId %{public}s", taskId_.c_str());
    downloadInfos_.clear();
}

ProgressInfo DownloadTaskProgressProcessor::GetTaskProgress()
{
    std::lock_guard<std::mutex> lockGuard(updateMutex_);
    // 先更新状态
    Update();

    ProgressInfo progressInfo;
    progressInfo.packageSize = packageSize_;
    progressInfo.downloadedSize = downloadedSize_;
    progressInfo.verifiedSize = verifiedSize_;
    progressInfo.taskProgress = CalcDownloadProgress();
    progressInfo.taskStatus = status_;
    progressInfo.reason = endReason_;

    for (const auto &downloadInfo : downloadInfos_) {
        if (downloadInfo == nullptr) {
            continue;
        }
        progressInfo.progresses.emplace_back(*downloadInfo);
    }
    ENGINE_LOGI("GetTaskProgress, taskId %{public}s, progress info %{public}s", taskId_.c_str(),
        progressInfo.ToString().c_str());
    return progressInfo;
}

void DownloadTaskProgressProcessor::Update()
{
    ENGINE_LOGD("Update task status");
    int64_t downloadSize = 0;
    int64_t verifiedSize = 0;
    std::map<DownloadStatus, int32_t> countMap;
    std::map<DownloadStatus, DownloadEndReason> endReasonMap;
    for (const auto &info : downloadInfos_) {
        if (info == nullptr) {
            continue;
        }
        downloadSize += info->downloadedSize;
        verifiedSize += info->verifiedSize;
        const auto &taskStatus = info->GetTaskStatus();
        if (!countMap.count(taskStatus)) {
            countMap[taskStatus] = 0;
        }
        countMap[taskStatus]++;
        endReasonMap[taskStatus] = info->endReason;
    }
    downloadedSize_ = downloadSize;
    verifiedSize_ = verifiedSize;

    // 如果都还未启动，不处理状态
    if (GetStatusCount(countMap, DownloadStatus::INIT) == totalInfoSize_) {
        return;
    }

    // 获取任务状态
    status_ = CalcDownloadTaskStatus(countMap);
    endReason_ = (status_ == DownloadStatus::SUCCESS) ? DownloadEndReason::SUCCESS : endReasonMap[status_];
}

int32_t DownloadTaskProgressProcessor::CalcDownloadProgress() const
{
    if (packageSize_ <= 0) {
        ENGINE_LOGE("CalcDownloadProgress error, packageSize %{public}" PRId64 " abnormal", packageSize_);
        return 0;
    }

    if (downloadedSize_ > packageSize_) {
        ENGINE_LOGE("CalcDownloadProgress error, downloadedSize %{public}" PRId64 " exceed packageSize%{public}" PRId64
            "",
            downloadedSize_, packageSize_);
        return DOWNLOAD_MAX_PERCENT;
    }
    return (int32_t)(downloadedSize_ * DOWNLOAD_MAX_PERCENT / packageSize_);
}

// FirmwareDownloadDataProcessor::CalcStatus的逻辑和本函数一致，如果本函数修改，要同步修改CalcStatus
DownloadStatus DownloadTaskProgressProcessor::CalcDownloadTaskStatus(
    const std::map<DownloadStatus, int32_t> &countMap) const
{
    // 全部下载成功，则任务成功
    if (GetStatusCount(countMap, DownloadStatus::SUCCESS) == totalInfoSize_) {
        return DownloadStatus::SUCCESS;
    }

    // 存在下载中任务，则整体下载中
    if (GetStatusCount(countMap, DownloadStatus::DOWNLOADING) || GetStatusCount(countMap, DownloadStatus::VERIFYING)) {
        return DownloadStatus::DOWNLOADING;
    }

    // 存在手动暂停任务
    if (GetStatusCount(countMap, DownloadStatus::PAUSE)) {
        return DownloadStatus::PAUSE;
    }

    // 存在自动暂停任务
    if (GetStatusCount(countMap, DownloadStatus::AUTO_PAUSE)) {
        return DownloadStatus::AUTO_PAUSE;
    }

    // 存在取消任务
    if (GetStatusCount(countMap, DownloadStatus::CANCEL)) {
        return DownloadStatus::CANCEL;
    }

    // 存在失败任务，则任务失败
    if (GetStatusCount(countMap, DownloadStatus::FAIL)) {
        return DownloadStatus::FAIL;
    }

    // 不在上述状态之内，默认返回下载中
    return DownloadStatus::DOWNLOADING;
}

int32_t DownloadTaskProgressProcessor::GetStatusCount(const std::map<DownloadStatus, int32_t> &countMap,
    DownloadStatus status) const
{
    if (!countMap.count(status)) {
        return 0;
    }
    return countMap.at(status);
}
} // namespace OHOS::UpdateService
