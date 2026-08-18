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

#include "download_service.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#include "async_task_emitter.h"
#include "download_common.h"
#include "download_info_operator.h"
#include "download_thread.h"
#include "dupdate_net_manager.h"
#include "file_utils.h"
#include "string_utils.h"
#include "time_utils.h"
#include "update_define.h"
#include "update_log.h"

namespace OHOS::UpdateService {
DownloadService::DownloadService(const std::string &taskId,
    const std::map<std::string, std::shared_ptr<DownloadInfo>> &downloadInfoMap)
{
    ENGINE_LOGI("DownloadService, taskId %{public}s", taskId.c_str());
    taskId_ = taskId;
    serviceDownloadInfoMap_ = downloadInfoMap;
}

DownloadService::~DownloadService()
{
    ENGINE_LOGI("DownloadService destructor, taskId %{public}s", taskId_.c_str());
}

DownloadResult DownloadService::Start(OnDownloadCallback callback, DownloadError &downloadError)
{
    if (serviceDownloadInfoMap_.empty()) {
        ENGINE_LOGE("Start failed, download info is empty, taskId %{public}s", taskId_.c_str());
        downloadError.build(DownloadEndReason::DOWNLOAD_INFO_EMPTY, DownloadCallResult::FAIL);
        return DownloadResult::FAIL;
    }

    if (!IsNetSatisfied()) {
        ENGINE_LOGE("Start failed, netType is not match, taskId %{public}s", taskId_.c_str());
        downloadError.build(DownloadEndReason::NET_NOT_AVAILIABLE, DownloadCallResult::FAIL);
        return DownloadResult::FAIL;
    }

    ENGINE_LOGI("Start, taskId %{public}s", taskId_.c_str());
    onDownloadCallback_ = std::move(callback);

    isStopCallback_.store(false, std::memory_order_release);
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    for (const auto &[downloadId, downloadInfo] : serviceDownloadInfoMap_) {
        if (downloadInfo == nullptr) {
            ENGINE_LOGE("start download thread fail, downloadId = %{public}s", downloadId.c_str());
            continue;
        }

        if (threadMap_.count(downloadInfo->downloadId)) {
            ENGINE_LOGE("thread already exist, downloadId %{public}s", downloadInfo->downloadId.c_str());
            continue;
        }

        if (downloadInfo->IsCompleted()) {
            ENGINE_LOGE("already download completed, downloadId %{public}s", downloadInfo->downloadId.c_str());
            continue;
        }

        // 首次下载场景下，将数据插入下载数据库
        DownloadInfoOperator().Insert(*downloadInfo);

        // 启动下载线程
        StartDownloadThread(*downloadInfo);
    }
    return DownloadResult::SUCCESS;
}

DownloadResult DownloadService::Pause(DownloadError &downloadError)
{
    ENGINE_LOGI("Pause download, taskId %{public}s", taskId_.c_str());
    // 停掉下载线程
    StopAllDownloadThread();

    // 回调暂停事件
    NotifyPauseCompleted();

    downloadError.build(DownloadEndReason::SUCCESS, DownloadCallResult::SUCCESS);
    return DownloadResult::SUCCESS;
}

DownloadResult DownloadService::Resume(NetType netType, DownloadError &downloadError)
{
    if (serviceDownloadInfoMap_.empty()) {
        ENGINE_LOGE("Resume failed, download info is empty, taskId %{public}s", taskId_.c_str());
        downloadError.build(DownloadEndReason::DOWNLOAD_INFO_EMPTY, DownloadCallResult::FAIL);
        return DownloadResult::FAIL;
    }

    isStopCallback_.store(false, std::memory_order_release);
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    // 刷新网络类型到数据库
    UpdateDownloadNetType(netType);
    if (!IsNetSatisfied()) {
        ENGINE_LOGE("Resume fail, netType is invalid, required netType %{public}d, curNetType %{public}d",
            CAST_INT(netType), CAST_INT(NetManager::GetInstance()->GetNetType()));
        downloadError.build(DownloadEndReason::NET_NOT_AVAILIABLE, DownloadCallResult::FAIL);
        return DownloadResult::FAIL;
    }

    for (const auto &[downloadId, downloadInfo] : serviceDownloadInfoMap_) {
        if (downloadInfo == nullptr) {
            ENGINE_LOGE("Resume skip, download info is null, downloadId %{public}s", downloadId.c_str());
            continue;
        }

        // 下载线程已存在，不进行恢复
        if (threadMap_.count(downloadId)) {
            ENGINE_LOGI("Resume skip, thread already exist, downloadId %{public}s", downloadId.c_str());
            continue;
        }

        // 下载已经完成，不进行恢复
        if (downloadInfo->IsCompleted()) {
            ENGINE_LOGI("Resume skip, already download completed, downloadId %{public}s", downloadId.c_str());
            continue;
        }
        if (downloadInfo->IsPaused()) {
            downloadInfo->status = DownloadStatus::DOWNLOADING;
        }
        // 刷新重试次数，避免重试失败时无法尝试问题
        downloadInfo->retryTimes = 0;
        StartDownloadThread(*downloadInfo);
    }
    downloadError.build(DownloadEndReason::SUCCESS, DownloadCallResult::SUCCESS);
    ENGINE_LOGI("Resume success, taskId %{public}s", taskId_.c_str());
    return DownloadResult::SUCCESS;
}

DownloadResult DownloadService::Cancel(DownloadError &downloadError, bool isDeleteFiles)
{
    ENGINE_LOGI("Cancel download, taskId %{public}s, isDeleteFiles %{public}d", taskId_.c_str(), isDeleteFiles);
    // 停掉下载线程
    StopAllDownloadThread();

    // 删除已下载文件
    if (isDeleteFiles) {
        DeleteDownloadedFiles();
    }

    // 回调取消事件
    NotifyCancelCompleted();
    return DownloadResult::SUCCESS;
}

void DownloadService::StopAllDownloadThread()
{
    ENGINE_LOGI("StopAllDownloadThread, taskId %{public}s", taskId_.c_str());
    isStopCallback_.store(true, std::memory_order_release);
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    // 取消所有还未启动的并行队列任务
    CancelUnexecutedTask();

    // 移除下载线程
    AsyncTaskEmitter emitter("StopAllDownloadThread_" + taskId_);
    for (const auto &[downloadId, downloadThread] : threadMap_) {
        if (downloadThread == nullptr) {
            ENGINE_LOGE("StopAllDownloadThread, download thread is null, downloadId %{public}s", downloadId.c_str());
            continue;
        }

        // 提交三个异步取消任务，减少取消等待时间
        ENGINE_LOGI("StopAllDownloadThread, stop download thread, downloadId %{public}s", downloadId.c_str());
        const auto &asyncTask = [thread = downloadThread]() { thread->Stop(); };
        emitter.Emit(asyncTask);
    }
    emitter.Wait();
    // 等待所有线程取消完成再清除数据
    threadMap_.clear();
}

void DownloadService::DeleteDownloadedFiles()
{
    ENGINE_LOGI("DeleteDownloadedFiles, taskId %{public}s", taskId_.c_str());
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    for (const auto &[downloadId, downloadInfo] : serviceDownloadInfoMap_) {
        if (downloadInfo == nullptr) {
            ENGINE_LOGE("Cancel download error, downloadInfo is null, downloadId %{public}s", downloadId.c_str());
            continue;
        }

        // 提交异步删除任务，减少取消等待时间
        ENGINE_LOGI("Cancel download, delete download file, downloadId %{public}s", downloadId.c_str());
        ClearPackage(downloadInfo->path);
    }
    // 等待所有文件删除完成再清除数据
}

void DownloadService::NotifyCancelCompleted()
{
    ENGINE_LOGI("NotifyCancelCompleted, taskId %{public}s", taskId_.c_str());
    for (const auto &[downloadId, downloadInfo] : serviceDownloadInfoMap_) {
        // 组装取消回调信息
        DownloadCallbackInfo callbackInfo;
        callbackInfo.downloadId = downloadInfo->downloadId;
        callbackInfo.downloadedSize = downloadInfo->downloadedSize;
        callbackInfo.endTime = TimeUtils::GetTimestamp();
        callbackInfo.status = DownloadStatus::CANCEL;
        callbackInfo.endReason = DownloadEndReason::CANCEL;
        callbackInfo.verifiedSize = downloadInfo->verifiedSize;

        // 通知任务取消
        UpdateDownloadProgress(callbackInfo);
    }
    // 删除数据库中任务
    DownloadInfoOperator().DeleteByTaskId(taskId_);
}

void DownloadService::NotifyPauseCompleted()
{
    ENGINE_LOGI("NotifyPauseCompleted, taskId %{public}s", taskId_.c_str());
    for (const auto &[downloadId, downloadInfo] : serviceDownloadInfoMap_) {
        if (downloadInfo == nullptr) {
            ENGINE_LOGE("NotifyPauseCompleted, skip empty download task");
            continue;
        }

        // 组装暂停回调信息
        DownloadCallbackInfo callbackInfo;
        callbackInfo.downloadId = downloadInfo->downloadId;
        callbackInfo.downloadedSize = downloadInfo->downloadedSize;
        callbackInfo.status = DownloadStatus::PAUSE;
        callbackInfo.endReason = DownloadEndReason::PAUSE;
        callbackInfo.verifiedSize = downloadInfo->verifiedSize;

        // 通知任务暂停
        UpdateDownloadProgress(callbackInfo);
    }
}

void DownloadService::NotifyRecoverCompleted()
{
    ENGINE_LOGI("NotifyRecoverCompleted, taskId %{public}s", taskId_.c_str());
    for (const auto &[downloadId, downloadInfo] : serviceDownloadInfoMap_) {
        if (downloadInfo == nullptr) {
            ENGINE_LOGE("NotifyRecoverCompleted, skip empty download task");
            continue;
        }

        // 已下载完成的不用再通知
        if (downloadInfo->IsCompleted()) {
            ENGINE_LOGI(
                "NotifyRecoverCompleted, skip complete download task, task status %{public}d, downloadId %{public}s",
                CAST_INT(downloadInfo->status), downloadInfo->downloadId.c_str());
            continue;
        }

        // 手动暂停的不需要改变状态
        if (downloadInfo->IsManualPaused()) {
            ENGINE_LOGI("NotifyRecoverCompleted, skip manual paused task, downloadId %{public}s",
                downloadInfo->downloadId.c_str());
            continue;
        }

        ENGINE_LOGI("NotifyRecoverCompleted, original status %{public}d, downloadId %{public}s",
            CAST_INT(downloadInfo->status), downloadInfo->downloadId.c_str());
        // 组装恢复回调信息, 通知上游状态更改为 AUTO_PAUSE
        DownloadCallbackInfo callbackInfo;
        callbackInfo.downloadId = downloadInfo->downloadId;
        callbackInfo.downloadedSize = downloadInfo->downloadedSize;
        callbackInfo.status = DownloadStatus::AUTO_PAUSE;
        callbackInfo.endReason = DownloadEndReason::INIT;
        callbackInfo.verifiedSize = downloadInfo->verifiedSize;

        // 通知任务加载完成
        UpdateDownloadProgress(callbackInfo);
    }
}

void DownloadService::Recover(OnDownloadCallback callback)
{
    if (serviceDownloadInfoMap_.empty()) {
        ENGINE_LOGE("Recover fail, download info is empty");
        return;
    }
    onDownloadCallback_ = std::move(callback);

    // 回调Task恢复完成
    NotifyRecoverCompleted();
}

void DownloadService::RemoveDownloadTask(const std::string &taskId)
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    ENGINE_LOGI("RemoveDownloadTask, taskId = %{public}s", taskId.c_str());
    onDownloadCallback_ = nullptr;
    serviceDownloadInfoMap_.clear();

    CancelUnexecutedTask();
}

void DownloadService::StartDownloadThread(const DownloadInfo &downloadInfo)
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    const auto onDownloadCallback = [=](const DownloadCallbackInfo &info) {
        ENGINE_LOGI("DownloadService execute size: %{public}" PRId64 ", downloadId : %{public}s", info.downloadedSize,
            info.downloadId.c_str());
        CallbackUpdateDownloadProgress(info);
    };

    auto downloadThread = std::make_shared<DownloadThread>(downloadInfo, onDownloadCallback);
    threadMap_[downloadInfo.downloadId] = downloadThread;
    downloadThread->Start();
}

DownloadResult DownloadService::HandleRetry(const std::string &downloadId, DownloadEndReason endReason)
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    auto downloadInfo = GetDownloadInfo(downloadId);
    if (downloadInfo == nullptr) {
        ENGINE_LOGE("HandleRetry abort, download info empty, downloadId %{public}s", downloadId.c_str());
        return DownloadResult::FAIL;
    }

    downloadInfo->retryTimes++;
    DownloadInfoOperator().UpdateRetryTimesById(*downloadInfo);
    ENGINE_LOGI("HandleRetry, start retry download, retryTimes: %{public}d, downloadId %{public}s",
        downloadInfo->retryTimes, downloadId.c_str());

    if (endReason == DownloadEndReason::REDIRECT || endReason == DownloadEndReason::SIZE_ERROR) {
        std::string switchUrl = downloadInfo->url;
        downloadInfo->url = downloadInfo->reserveUrl.empty() ? switchUrl : downloadInfo->reserveUrl;
        ENGINE_LOGI("HandleRetry switch url");
        downloadInfo->reserveUrl = switchUrl;
    }

    StartDownloadThread(*downloadInfo);
    return DownloadResult::SUCCESS;
}

void DownloadService::NetChangeToPause()
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    // 取消并发队列中尚未执行的任务
    CancelUnexecutedTask();

    // 通知正在运行的下载线程暂停下载
    for (const auto &[downloadId, downloadThread] : threadMap_) {
        if (downloadThread == nullptr) {
            ENGINE_LOGE("NetChangeToPause abort, download thread not exist, downloadId %{public}s", downloadId.c_str());
            continue;
        }
        ENGINE_LOGI("NetChangeToPause, downloadId is %{public}s", downloadId.c_str());
        downloadThread->NetChangeToPause();
    }
}

void DownloadService::NetChangeToStart()
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    for (const auto &[downloadId, downloadInfo] : serviceDownloadInfoMap_) {
        if (downloadInfo == nullptr) {
            ENGINE_LOGE("NetChangeToStart abort, download info empty, downloadId %{public}s", downloadId.c_str());
            continue;
        }

        if (downloadInfo->status != DownloadStatus::AUTO_PAUSE || !downloadInfo->isNeedAutoResume) {
            ENGINE_LOGE("NetChangeToStart abort, download status %{public}d, isNeedAutoResume %{public}s",
                downloadInfo->status, StringUtils::GetBoolStr(downloadInfo->isNeedAutoResume).c_str());
            continue;
        }

        if (threadMap_.count(downloadId) && threadMap_.at(downloadId) != nullptr) {
            ENGINE_LOGE("NetChangeToStart abort, thread is running, downloadId is %{public}s", downloadId.c_str());
            continue;
        }

        // 重启下载线程
        ENGINE_LOGI("NetChangeToStart, restart download thread, downloadId %{public}s", downloadId.c_str());
        StartDownloadThread(*downloadInfo);
    }
}

void DownloadService::CallbackDownloadProgress(const DownloadCallbackInfo &info)
{
    // 回调进度
    if (onDownloadCallback_ == nullptr) {
        ENGINE_LOGE("Callback member is nullptr");
        return;
    }
    onDownloadCallback_(info);
}

void DownloadService::CallbackUpdateDownloadProgress(DownloadCallbackInfo info)
{
    if (isStopCallback_.load(std::memory_order_acquire)) {
        ENGINE_LOGI("download thread is stop");
        return;
    }
    UpdateDownloadProgress(info);
}

void DownloadService::UpdateDownloadProgress(DownloadCallbackInfo info)
{
    // 非下载中、校验中状态，下载线程已经结束，移除下载线程
    if (info.status != DownloadStatus::DOWNLOADING && info.status != DownloadStatus::VERIFYING) {
        RemoveThread(info.downloadId);
    }

    // 非失败场景下，无需进行重试，直接向上层回调进度
    if (info.status != DownloadStatus::FAIL) {
        UpdateDownloadInfo(info);
        CallbackDownloadProgress(info);
        return;
    }

    // 失败场景下，根据失败情况进行重试下载
    const auto downloadInfo = GetDownloadInfo(info.downloadId);
    if (downloadInfo == nullptr) {
        ENGINE_LOGE("UpdateDownloadProgress abort, download info not found, downloadId %{public}s",
            info.downloadId.c_str());
        return;
    }

    // 写入磁盘失败或者sha256校验失败，不重试下载
    if (info.endReason == DownloadEndReason::IO_EXCEPTION || info.endReason == DownloadEndReason::VERIFY_FAIL) {
        ENGINE_LOGE("UpdateDownloadProgress, delete download file, downloadId %{public}s, fail reason %{public}d",
            downloadInfo->downloadId.c_str(), CAST_INT(info.endReason));
        ClearPackage(downloadInfo->path);
        UpdateDownloadInfo(info);
        CallbackDownloadProgress(info);
        return;
    }

    // 不能再重试下载时，刷新并上报最终状态
    if (!downloadInfo->IsRetryAble()) {
        ENGINE_LOGI("UpdateDownloadProgress, endReason: %{public}d , retryTimes: %{public}d", CAST_INT(info.endReason),
            downloadInfo->retryTimes);
        UpdateDownloadInfo(info);
        CallbackDownloadProgress(info);
        return;
    }

    // 重试下载失败时，刷新并上报最终状态
    if (HandleRetry(info.downloadId, info.endReason) != DownloadResult::SUCCESS) {
        ENGINE_LOGE("UpdateDownloadProgress, retry fail, endReason: %{public}d , retryTimes: %{public}d",
            CAST_INT(info.endReason), downloadInfo->retryTimes);
        UpdateDownloadInfo(info);
        CallbackDownloadProgress(info);
        return;
    }

    ENGINE_LOGI("UpdateDownloadProgress, retry success, endReason: %{public}d , retryTimes: %{public}d",
        CAST_INT(info.endReason), downloadInfo->retryTimes);
}

std::shared_ptr<DownloadInfo> DownloadService::UpdateDownloadInfo(const DownloadCallbackInfo &info)
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    if (!serviceDownloadInfoMap_.count(info.downloadId)) {
        ENGINE_LOGE("no download info member, downloadId is %{public}s", info.downloadId.c_str());
        return nullptr;
    }

    auto downloadInfo = serviceDownloadInfoMap_.at(info.downloadId);
    if (downloadInfo == nullptr) {
        ENGINE_LOGE("download info is empty");
        return nullptr;
    }

    if (info.status == DownloadStatus::DOWNLOADING && isStopCallback_.load(std::memory_order_acquire)) {
        ENGINE_LOGE("status is error");
        return nullptr;
    }

    // 刷新内存中下载进度
    downloadInfo->UpdateDownloadInfo(info);

    if (info.status != DownloadStatus::CANCEL) {
        // 更新数据库中下载进度
        DownloadInfoOperator().UpdateProgressById(*downloadInfo);
    }
    return downloadInfo;
}

void DownloadService::RemoveThread(const std::string &downloadId)
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    threadMap_.erase(downloadId);
}

void DownloadService::SetDisableCallbackFlag(bool flag)
{
    // 通知各个线程停止回调下载进度
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    for (const auto &[downloadId, downloadThread] : threadMap_) {
        ENGINE_LOGI("DownloadService::NotifyCallbackDisabled downloadId = %{public}s, flag = %{public}d",
            downloadId.c_str(), flag);
        downloadThread->SetDisableCallbackFlag(flag);
    }
}

bool DownloadService::IsNetSatisfied()
{
    for (const auto &[downloadId, downloadInfo] : serviceDownloadInfoMap_) {
        if (downloadInfo == nullptr) {
            continue;
        }
        return NetManager::GetInstance()->IsNetAvailable(downloadInfo->netType);
    }
    return false;
}

DownloadResult DownloadService::ClearPackage(const std::string &path)
{
    struct stat statBuf {};
    int ret = lstat(path.c_str(), &statBuf);
    if (ret != 0) {
        ENGINE_LOGE("ClearPackage fail, lstat errno %{public}d", errno);
        return DownloadResult::FAIL;
    }

    if (!S_ISREG(statBuf.st_mode)) {
        ENGINE_LOGE("ClearPackage fail, package path is invalid path %{private}s", path.c_str());
        return DownloadResult::FAIL;
    }
    ret = remove(path.c_str());
    if (ret != 0) {
        ENGINE_LOGE("ClearPackage fail, remove file errno %{public}d", errno);
    }
    return DownloadResult::SUCCESS;
}

void DownloadService::UpdateDownloadNetType(NetType netType)
{
    DownloadInfoOperator downloadInfoOperator;
    for (const auto &[downloadId, downloadInfo] : serviceDownloadInfoMap_) {
        if (downloadInfo == nullptr) {
            ENGINE_LOGE("UpdateDownloadNetType skip, download info is null, downloadId %{public}s", downloadId.c_str());
            continue;
        }

        if (netType == downloadInfo->netType) {
            continue;
        }
        downloadInfo->netType = netType;
        downloadInfoOperator.UpdateNetTypeById(*downloadInfo);
    }
}

std::shared_ptr<DownloadInfo> DownloadService::GetDownloadInfo(const std::string &downloadId)
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadServiceMutex_);
    if (!serviceDownloadInfoMap_.count(downloadId)) {
        ENGINE_LOGE("GetDownloadInfo, no download info member, downloadId is %{public}s", downloadId.c_str());
        return nullptr;
    }

    return serviceDownloadInfoMap_.at(downloadId);
}
} // namespace OHOS::UpdateService