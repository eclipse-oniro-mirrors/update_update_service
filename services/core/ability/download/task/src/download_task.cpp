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

#include "download_task.h"

#include <thread>
#include <mutex>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include <sys/statfs.h>

#include "constant.h"
#include "download_service.h"
#include "download_info_operator.h"
#include "download_task_manager.h"
#include "dupdate_net_manager.h"
#include "file_utils.h"
#include "serial_async_task_emitter.h"
#include "sha256_utils.h"
#include "time_utils.h"
#include "update_log.h"

namespace OHOS::UpdateService {
constexpr size_t DOWNLOAD_MIN_PATH_LEN = 3;

DownloadTask::DownloadTask(const std::string &taskId, std::vector<DownloadInfo> &downloadInfos)
{
    ENGINE_LOGI("DownloadTask, taskId %{public}s", taskId.c_str());
    taskId_ = taskId;
    InitDownloadInfo(downloadInfos);

    downloadService_ = std::make_unique<DownloadService>(taskId, downloadInfoMap_);
}

DownloadTask::~DownloadTask()
{
    ENGINE_LOGI("DownloadTask destructor, taskId %{public}s", taskId_.c_str());
}

std::string DownloadTask::Start(DownloadCallback callback, DownloadError &downloadError)
{
    if (downloadInfoMap_.empty()) {
        ENGINE_LOGE("Start download failed, download info is empty, taskId %{public}s", taskId_.c_str());
        downloadError.build(DownloadEndReason::DOWNLOAD_INFO_EMPTY, DownloadCallResult::FAIL);
        return "";
    }

    ENGINE_LOGI("Start download, taskId %{public}s", taskId_.c_str());
    callback_ = std::move(callback);
    if (!IsMemoryEnough(progressInfo_.packageSize - progressInfo_.downloadedSize)) {
        ENGINE_LOGE("Start download failed, download space is not enough, taskId %{public}s", taskId_.c_str());
        downloadError.build(DownloadEndReason::NO_ENOUGH_MEMORY, DownloadCallResult::FAIL);
        return taskId_;
    }

    const auto weakThis = weak_from_this();
    OnDownloadCallback progressCallback = [weakThis, this](DownloadCallbackInfo info) {
        const auto sharedPtr = weakThis.lock();
        if (sharedPtr == nullptr) {
            ENGINE_LOGE("download progress callback abort, task expired, progress info %{public}s",
                info.ToString().c_str());
            return;
        }

        ENGINE_LOGI("download progress callback info %{public}s", info.ToString().c_str());
        UpdateTaskProgress(info);
    };

    if (downloadService_ == nullptr) {
        ENGINE_LOGE("Start failed, download service is null, taskId %{public}s", taskId_.c_str());
        downloadError.build(DownloadEndReason::FAIL, DownloadCallResult::FAIL);
        return taskId_;
    }

    // 启动下载任务
    downloadService_->Start(progressCallback, downloadError);

    // 起用进度回调
    EnableCallback();

    ENGINE_LOGI("Start download success, taskId %{public}s", taskId_.c_str());
    return taskId_;
}

void DownloadTask::Pause(DownloadError &downloadError)
{
    // 禁用回调
    isStopDownloadingCallback_.store(true, std::memory_order_release);
    DisableCallback();
    if (downloadService_ == nullptr) {
        ENGINE_LOGE("Pause failed, download service is null, taskId %{public}s", taskId_.c_str());
        downloadError.build(DownloadEndReason::FAIL, DownloadCallResult::FAIL);
        return;
    }

    ENGINE_LOGI("Pause download, taskId %{public}s", taskId_.c_str());
    // 暂停下载任务
    downloadService_->Pause(downloadError);
    ENGINE_LOGI("Pause download success, taskId %{public}s", taskId_.c_str());
}

void DownloadTask::Resume(NetType netType, DownloadCallback callback, DownloadError &downloadError)
{
    // 如果存在下载失败的任务，重置状态以重试下载
    for (auto &[downloadId, downloadInfo] : downloadInfoMap_) {
        if (downloadInfo->status == DownloadStatus::FAIL) {
            downloadInfo->status = DownloadStatus::INIT;
        }
    }

    // 设置并起用回调
    callback_ = std::move(callback);
    EnableCallback();

    if (downloadService_ == nullptr) {
        ENGINE_LOGE("Resume failed, download service is null, taskId %{public}s", taskId_.c_str());
        downloadError.build(DownloadEndReason::FAIL, DownloadCallResult::FAIL);
        return;
    }

    (void)downloadService_->Resume(netType, downloadError);

    std::lock_guard<ffrt::recursive_mutex> lockGuard(downloadProgressUpdateMutex_);
    const auto progressInfo = GetTaskProgress();
    progressInfo_ = progressInfo;
    status_ = progressInfo.taskStatus;
    ENGINE_LOGI("Resume download, taskId %{public}s, netType %{public}d, ret %{public}d, reason %{public}d",
        taskId_.c_str(), CAST_INT(netType), CAST_INT(downloadError.errorNum), CAST_INT(downloadError.endReason));
}

void DownloadTask::Cancel(DownloadError &downloadError, bool isDeleteFiles)
{
    // 取消进度回调
    isStopDownloadingCallback_.store(true, std::memory_order_release);
    DisableCallback();
    if (downloadService_ == nullptr) {
        ENGINE_LOGI("Cancel abort, download service is null, taskId %{public}s", taskId_.c_str());
        return;
    }

    ENGINE_LOGI("Cancel task, taskId %{public}s", taskId_.c_str());
    downloadService_->Cancel(downloadError, isDeleteFiles);

    // 置空callback，不再允许回调
    callback_ = nullptr;

    // 构建取消成功结果，上报OUC
    downloadError.build(DownloadEndReason::SUCCESS, DownloadCallResult::SUCCESS);
    ENGINE_LOGI("Cancel task end, taskId %{public}s", taskId_.c_str());
}

void DownloadTask::Recover()
{
    if (downloadInfoMap_.empty()) {
        ENGINE_LOGE("Recover fail, download info is empty, taskId %{public}s", taskId_.c_str());
        return;
    }

    taskId_ = GetTaskId();
    ENGINE_LOGI("Recover, download status %{public}d", status_);

    if (downloadService_ == nullptr) {
        ENGINE_LOGE("Recover failed, download service is null, taskId %{public}s", taskId_.c_str());
        return;
    }

    const auto weakThis = weak_from_this();
    OnDownloadCallback onDownloadCallback = [weakThis, this](DownloadCallbackInfo info) {
        const auto sharedPtr = weakThis.lock();
        if (sharedPtr == nullptr) {
            ENGINE_LOGE("download progress callback abort, task expired, progress info %{public}s",
                info.ToString().c_str());
            return;
        }

        ENGINE_LOGI("download CallbackProgress downloadedSize %{public}" PRId64 ", status %{public}d, "
            "downloadId %{public}s",
            info.downloadedSize, info.status, info.downloadId.c_str());
        UpdateTaskProgress(info);
    };
    downloadService_->Recover(onDownloadCallback);
}

ProgressInfo DownloadTask::GetTaskProgress()
{
    if (taskProgressProcessor_ == nullptr) {
        return {};
    }
    return taskProgressProcessor_->GetTaskProgress();
}

void DownloadTask::CallbackProgress(const ProgressInfo &progressInfo)
{
    ENGINE_LOGD("CallbackProgress get mutex start");
    std::unique_lock<ffrt::recursive_mutex> lock(callbackMutex_);
    ENGINE_LOGD("CallbackProgress get mutex end");
    CallbackProgressLocked(progressInfo);
}

void DownloadTask::CallbackProgressLocked(const ProgressInfo &progressInfo)
{
    if (callback_ == nullptr) {
        ENGINE_LOGE("CallbackProgress abort, callback is nullptr, taskId %{public}s, progress info %{public}s",
            taskId_.c_str(), progressInfo.ToString().c_str());
        return;
    }

    if (!progressInfo.IsSyncCallback() && isStopDownloadingCallback_.load(std::memory_order_acquire)) {
        ENGINE_LOGE("CallbackProgress fail, taskId %{public}s, progress info %{public}s", taskId_.c_str(),
            progressInfo.ToString().c_str());
        return;
    }
    ENGINE_LOGI("CallbackProgress, taskId %{public}s, progress info %{public}s", taskId_.c_str(),
        progressInfo.ToString().c_str());
    callback_(taskId_, progressInfo);
    lastReportTime_ = TimeUtils::GetTimestampByMilliseconds();
}

void DownloadTask::DestroyTask()
{
    if (downloadService_ == nullptr) {
        ENGINE_LOGE("DestroyTask abort, download service is null, taskId %{public}s", taskId_.c_str());
        return;
    }
    ENGINE_LOGE("DestroyTask, taskId %{public}s", taskId_.c_str());
    downloadService_->RemoveDownloadTask(taskId_);
}

void DownloadTask::InitDownloadInfo(std::vector<DownloadInfo> &downloadInfos)
{
    std::vector<std::shared_ptr<DownloadInfo>> infoVector;
    for (auto &downloadInfo : downloadInfos) {
        downloadInfo.downloadId = downloadInfo.versionId;
        downloadInfo.taskId = taskId_;

        int64_t fileRealSize = FileUtils::GetFileSize(downloadInfo.path);
        ENGINE_LOGI("InitDownloadInfo, downloadId %{public}s, downloadedSize %{public}" PRId64
            ", fileRealSize %{public}" PRId64 ", packageSize %{public}" PRId64 "",
            downloadInfo.downloadId.c_str(), downloadInfo.downloadedSize, fileRealSize, downloadInfo.packageSize);
        downloadInfo.downloadedSize = fileRealSize;

        const auto sharedInfo = std::make_shared<DownloadInfo>(downloadInfo);
        infoVector.emplace_back(sharedInfo);
        downloadInfoMap_[downloadInfo.downloadId] = sharedInfo;
    }

    taskProgressProcessor_ = std::make_unique<DownloadTaskProgressProcessor>(infoVector);

    // 完成初始化，更新一次任务状态
    const auto progressInfo = GetTaskProgress();
    progressInfo_ = progressInfo;
    status_ = progressInfo.taskStatus;
    ENGINE_LOGI("InitDownloadInfo downloadedSize %{public}" PRId64 ", packageSize %{public}" PRId64 "",
        progressInfo.downloadedSize, progressInfo.packageSize);
}

bool DownloadTask::IsMemoryEnough(int64_t size)
{
    const auto rootPath = GetRootPath();
    ENGINE_LOGI("IsMemoryEnough, ota pkg path %{private}s", rootPath.c_str());
    if (rootPath.length() < DOWNLOAD_MIN_PATH_LEN) {
        ENGINE_LOGE("IsMemoryEnough, invalid path %{private}s", rootPath.c_str());
        return false;
    }

    if (!FileUtils::IsFileExist(rootPath)) {
        FileUtils::CreateMultiDirWithPermission(rootPath, FOLDER_PERMISSION);
    }
    return FileUtils::IsSpaceEnough(rootPath, size);
}

void DownloadTask::UpdateTaskProgress(const DownloadCallbackInfo &info)
{
    const auto progressInfo = GetTaskProgress();
    ENGINE_LOGI("UpdateTaskProgress, %{public}s", progressInfo.ToString().c_str());

    // 任务进度、状态未发生变化时，不回调进度，避免频繁回调
    if (!IsProgressChanged(progressInfo)) {
        ENGINE_LOGI("UpdateTaskProgress, IsProgressChanged is false");
        return;
    }

    // 任务结束、取消、暂停，取消进度回调
    if (progressInfo.IsTaskCompleted()) {
        CancelTimedTask();
    }

    if (progressInfo.IsSyncCallback()) {
        CallbackProgress(progressInfo);
        return;
    }

    // 提交异步进度上报任务
    const auto callbackTask = [weakThis = weak_from_this(), progressInfo, taskId = taskId_]() {
        const auto sharedThis = weakThis.lock();
        if (sharedThis == nullptr) {
            ENGINE_LOGI("UpdateTaskProgress, taskId %{public}s, discard progressInfo %{public}s", taskId.c_str(),
                progressInfo.ToString().c_str());
            return;
        }
        const auto currentProgressInfo = sharedThis->GetTaskProgress();
        if (currentProgressInfo.IsSyncCallback()) {
            ENGINE_LOGI("UpdateTaskProgress fail,current status: %{public}s", currentProgressInfo.ToString().c_str());
            return;
        }
        sharedThis->CallbackProgress(progressInfo);
    };
    SerialAsyncTaskQueueEmitter::GetInstance()->Submit(taskId_, callbackTask);
}

void DownloadTask::DisableCallback()
{
    ENGINE_LOGI("DisableCallback, taskId %{public}s", taskId_.c_str());
    NotifyCallbackDisabled();

    // 取消进度上报保护任务
    CancelTimedTask();
}

void DownloadTask::EnableCallback()
{
    ENGINE_LOGI("EnableCallback, taskId %{public}s", taskId_.c_str());
    NotifyCallbackDisabled(false);

    isStopDownloadingCallback_.store(false, std::memory_order_release);
    // 提交进度上报保护任务
    SubmitTimedTask();
}

void DownloadTask::NotifyCallbackDisabled(bool flag)
{
    if (downloadService_ == nullptr) {
        ENGINE_LOGE("NotifyCallbackDisabled abort, download service empty, taskId %{public}s", taskId_.c_str());
        return;
    }

    ENGINE_LOGI("NotifyCallbackDisabled, taskId %{public}s", taskId_.c_str());
    downloadService_->SetDisableCallbackFlag(flag);
}

std::string DownloadTask::GetRootPath() const
{
    for (const auto &[downloadId, downloadInfo] : downloadInfoMap_) {
        if (downloadInfo == nullptr) {
            continue;
        }
        if (downloadInfo->dirType == DownloadDirType::ENCRYPT_DIR) {
            return Constant::DUPDATE_ENGINE_ENCRYPTED_ROOT_PATH;
        }
        return Constant::DUPDATE_ENGINE_PACKAGE_ROOT_PATH;
    }
    return "";
}

std::string DownloadTask::GetTaskId() const
{
    for (const auto &[downloadId, downloadInfo] : downloadInfoMap_) {
        if (downloadInfo == nullptr) {
            continue;
        }
        return downloadInfo->taskId;
    }
    return "";
}

NetType DownloadTask::GetRequiredNetType() const
{
    for (const auto &[downloadId, downloadInfo] : downloadInfoMap_) {
        if (downloadInfo == nullptr) {
            continue;
        }
        return downloadInfo->netType;
    }
    return NetType::NOT_METERED_WIFI;
}

void DownloadTask::OnNetChange(NetType netType)
{
    if (downloadService_ == nullptr) {
        ENGINE_LOGE("OnNetChange abort, downloader is null, taskId %{public}s", taskId_.c_str());
        return;
    }

    ENGINE_LOGI("OnNetChange, net type %{public}d, taskId %{public}s", CAST_INT(netType), taskId_.c_str());
    if (NetManager::GetInstance()->IsNetAvailable(GetRequiredNetType())) {
        EnableCallback();
        downloadService_->NetChangeToStart();
        return;
    }

    // 网络条件不符合，暂停回调，停掉下载线程
    DisableCallback();
    downloadService_->NetChangeToPause();
}

void DownloadTask::SubmitTimedTask()
{
    auto weakThis = weak_from_this();
    auto worker = [weakThis, this]() {
        const auto sharedPtr = weakThis.lock();
        if (sharedPtr == nullptr) {
            ENGINE_LOGE("download task has been destroy");
            return;
        }

        // 提交的延时任务不能上报最终稳定状态，只能向上报下载中（校验中）的状态
        const auto progress = GetTaskProgress();
        if (progress.taskStatus == DownloadStatus::FAIL || progress.taskStatus == DownloadStatus::SUCCESS ||
            progress.taskStatus == DownloadStatus::CANCEL || progress.taskStatus == DownloadStatus::PAUSE ||
            progress.taskStatus == DownloadStatus::AUTO_PAUSE) {
            ENGINE_LOGI("Download completed, status %{public}d", CAST_INT(progress.taskStatus));
            return;
        }

        // 回调进度
        constexpr int64_t nextReportIntervalInMills = 10 * 1000;
        if ((TimeUtils::GetTimestampByMilliseconds() - lastReportTime_) >= nextReportIntervalInMills) {
            CallbackProgress(progress);
        }

        // 任务尚未结束，提交下一周期回调任务，先取消已存在的任务，避免存在多个任务
        CancelTimedTask();
        SubmitTimedTask();
    };
    const auto nextReportInterval = CalcNextReportInterval();
    ENGINE_LOGI("SubmitTimedTask, taskId %{public}s, next report interval %{public}" PRIu64 " micro seconds",
        taskId_.c_str(), nextReportInterval);
    timedTaskHandle_ = SerialAsyncTaskQueueEmitter::GetInstance()->Submit(taskId_, worker,
        ffrt::task_attr().delay(nextReportInterval));
}

void DownloadTask::CancelTimedTask()
{
    ENGINE_LOGI("CancelTimedTask, taskId %{public}s", taskId_.c_str());
    SerialAsyncTaskQueueEmitter::GetInstance()->Cancel(taskId_, timedTaskHandle_);
}

bool DownloadTask::IsProgressChanged(const ProgressInfo &progressInfo)
{
    std::lock_guard<ffrt::recursive_mutex> lockGuard(downloadProgressUpdateMutex_);
    // 最新进度比当前已记录进度小，丢弃掉
    if (progressInfo.taskStatus == status_ && (progressInfo.downloadedSize < progressInfo_.downloadedSize ||
        progressInfo.verifiedSize < progressInfo_.verifiedSize)) {
        ENGINE_LOGE("IsProgressChanged, discard invalid progress info, taskId %{public}s, %{public}s", taskId_.c_str(),
            progressInfo.ToString().c_str());
        return false;
    }

    // 状态已经上报过，不再继续上报
    if (progressInfo.taskStatus == status_ && progressInfo.IsTaskCompleted()) {
        ENGINE_LOGE("IsProgressChanged, discard duplicate progress, taskId %{public}s, %{public}s", taskId_.c_str(),
            progressInfo.ToString().c_str());
        return false;
    }

    bool isChanged = progressInfo.taskProgress != reportedTaskProgress_ || progressInfo.taskStatus != status_ ||
        progressInfo.verifiedSize != progressInfo_.verifiedSize ||
        progressInfo.downloadedSize != progressInfo_.downloadedSize;
    reportedTaskProgress_ = progressInfo.taskProgress;
    status_ = progressInfo.taskStatus;
    progressInfo_ = progressInfo;
    return isChanged;
}

uint64_t DownloadTask::CalcNextReportInterval() const
{
    constexpr int64_t reportInterval = 1000 * 1000 * 10; // 单位：微秒
    constexpr int64_t millsToMicro = 1000;

    // 先计算出距离上次上报进度已经过了多长时间 TD
    int64_t lastReportDuration = (TimeUtils::GetTimestampByMilliseconds() - lastReportTime_) * millsToMicro;
    // 如果TD<=0，说明刚上报过，则下一次10秒之后上报
    if (lastReportDuration <= 0) {
        return static_cast<uint64_t>(reportInterval);
    }

    // 如果TD>=10秒，需要立即上报一次
    auto nextReportInterval = reportInterval - lastReportDuration;
    if (nextReportInterval <= 0) {
        return 0;
    }
    return static_cast<uint64_t>(nextReportInterval);
}

bool DownloadTask::IsConcurrentLimited(std::vector<DownloadInfo> &downloadInfos)
{
    for (const auto &downloadInfo : downloadInfos) {
        if (downloadInfo.isConcurrentLimited) {
            return true;
        }
    }
    return false;
}
} // namespace OHOS::UpdateService