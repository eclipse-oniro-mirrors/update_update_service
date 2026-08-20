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

#include "download_task_manager.h"

#include <thread>
#include <mutex>
#include <unistd.h>
#include <unordered_set>
#include <string>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <utility>
#include <vector>
#include <algorithm>

#include "download_info_operator.h"
#include "download_service.h"
#include "download_task.h"
#include "dupdate_net_manager.h"
#include "sha256_utils.h"
#include "time_utils.h"
#include "update_log.h"

namespace OHOS::UpdateService {
DownloadTaskManager::DownloadTaskManager()
{
    ENGINE_LOGI("DownloadTaskManager");
    Init();
}

DownloadTaskManager::~DownloadTaskManager()
{
    ENGINE_LOGI("~DownloadTaskManager");
}

std::string DownloadTaskManager::Start(std::vector<DownloadInfo> &downloadInfos, DownloadCallback callback,
    DownloadError &downloadError, DownloadDirType dirType)
{
    if (downloadInfos.empty()) {
        ENGINE_LOGE("Start fail, download info is empty");
        downloadError.build(DownloadEndReason::DOWNLOAD_INFO_EMPTY, DownloadCallResult::FAIL);
        return "";
    }

    std::lock_guard<ffrt::recursive_mutex> guard(downloadTaskMapMutex_);

    // 对versionId进行虑重
    auto newDownloadInfos = RemoveDuplicateVersionId(downloadInfos);

    std::string taskId = GetTaskId(newDownloadInfos);
    std::shared_ptr<DownloadTask> dirtyTask = nullptr;
    if (downloadTaskMap_.Find(taskId, dirtyTask)) {
        downloadTaskMap_.Erase(taskId);
        if (dirtyTask != nullptr) {
            ENGINE_LOGI("Start, destroy dirty task, taskId %{public}s", taskId.c_str());
            dirtyTask->Cancel(downloadError);
            dirtyTask->DestroyTask();
        }
    }

    DownloadCallback taskCallBack = [callback, this](const std::string &taskId, ProgressInfo progressInfo) {
        DealCallBack(taskId, progressInfo, callback);
    };

    const auto task = std::make_shared<DownloadTask>(taskId, newDownloadInfos);
    task->Start(taskCallBack, downloadError);

    // 下载启动失败，移除任务
    if (downloadError.errorNum != DownloadCallResult::SUCCESS) {
        ENGINE_LOGI("Start abort, download task has been started, taskId %{public}s", taskId.c_str());
        task->DestroyTask();
        return taskId;
    }

    // 保存下载任务
    downloadTaskMap_.EnsureInsert(taskId, task);
    return taskId;
}

std::vector<DownloadInfo> DownloadTaskManager::RemoveDuplicateVersionId(const std::vector<DownloadInfo> &downloadInfos)
{
    ENGINE_LOGI("DownloadTaskManager::removeDuplicateVersionId");
    std::vector<DownloadInfo> tempDownloadInfos;
    std::unordered_set<std::string> currentVersionIds;
    for (const auto &downloadInfo : downloadInfos) {
        if (currentVersionIds.count(downloadInfo.versionId)) {
            continue;
        }
        currentVersionIds.emplace(downloadInfo.versionId);
        tempDownloadInfos.emplace_back(downloadInfo);
    }
    return tempDownloadInfos;
}

void DownloadTaskManager::Pause(std::string taskId, DownloadError &downloadError)
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadTaskMapMutex_);
    std::shared_ptr<DownloadTask> task = nullptr;
    if (!GetValidTask(taskId, task)) {
        ENGINE_LOGE("Pause abort, task invalid, taskId %{public}s", taskId.c_str());
        downloadError.build(DownloadEndReason::DOWNLOAD_INFO_EMPTY, DownloadCallResult::FAIL);
        return;
    }

    task->Pause(downloadError);
}

void DownloadTaskManager::Resume(std::string taskId, NetType netType, DownloadCallback callback,
    DownloadError &downloadError)
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadTaskMapMutex_);
    std::shared_ptr<DownloadTask> task = nullptr;
    if (!GetValidTask(taskId, task)) {
        ENGINE_LOGE("Resume abort, task invalid, taskId %{public}s", taskId.c_str());
        downloadError.build(DownloadEndReason::DOWNLOAD_INFO_EMPTY, DownloadCallResult::FAIL);
        return;
    }

    DownloadCallback taskCallBack = [callback, this](const std::string &taskId, ProgressInfo progressInfo) {
        DealCallBack(taskId, progressInfo, callback);
    };

    task->Resume(netType, taskCallBack, downloadError);
}

void DownloadTaskManager::Cancel(std::string taskId, DownloadError &downloadError, bool isDeleteFiles)
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadTaskMapMutex_);
    std::shared_ptr<DownloadTask> task = nullptr;
    if (!GetValidTask(taskId, task)) {
        ENGINE_LOGE("Cancel abort, task invalid, taskId %{public}s", taskId.c_str());
        downloadError.build(DownloadEndReason::DOWNLOAD_INFO_EMPTY, DownloadCallResult::FAIL);
        return;
    }

    // 取消下载任务
    task->Cancel(downloadError, isDeleteFiles);
}

void DownloadTaskManager::RemoveTask(std::string taskId, DownloadError &downloadError)
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadTaskMapMutex_);
    DownloadInfoOperator().DeleteByTaskId(taskId);
    std::shared_ptr<DownloadTask> task = nullptr;
    if (!GetValidTask(taskId, task)) {
        ENGINE_LOGE("RemoveTask abort, task invalid, taskId %{public}s", taskId.c_str());
        downloadError.build(DownloadEndReason::DOWNLOAD_INFO_EMPTY, DownloadCallResult::FAIL);
        return;
    }

    ProgressInfo progressInfo = GetTaskProgress(taskId, downloadError);
    if (progressInfo.taskStatus != DownloadStatus::CANCEL && progressInfo.taskStatus != DownloadStatus::FAIL &&
        progressInfo.taskStatus != DownloadStatus::PAUSE && progressInfo.taskStatus != DownloadStatus::AUTO_PAUSE &&
        progressInfo.taskStatus != DownloadStatus::SUCCESS) {
        ENGINE_LOGE("RemoveTask abort, taskStatus is wrong, taskId %{public}s, taskStatus %{public}d", taskId.c_str(),
            progressInfo.taskStatus);
        downloadError.build(DownloadEndReason::SYSTEM_BUSY, DownloadCallResult::FAIL);
        return;
    }

    ENGINE_LOGI("RemoveTask, taskId %{public}s", taskId.c_str());
    task->DestroyTask();
    downloadTaskMap_.Erase(taskId);
}

ProgressInfo DownloadTaskManager::GetTaskProgress(std::string taskId, DownloadError &downloadError)
{
    std::lock_guard<ffrt::recursive_mutex> guard(downloadTaskMapMutex_);
    std::shared_ptr<DownloadTask> task = nullptr;
    if (!GetValidTask(taskId, task)) {
        ENGINE_LOGE("GetTaskProgress fail, task invalid, taskId %{public}s", taskId.c_str());
        downloadError.build(DownloadEndReason::DOWNLOAD_INFO_EMPTY, DownloadCallResult::FAIL);
        return {};
    }
    return task->GetTaskProgress();
}

void DownloadTaskManager::Init()
{
    // 从数据库恢复未完成的下载任务
    RestoreDownloadTask();

    // 注册网络监听
    NetManager::GetInstance()->RegisterCallback(NetChangeCallbackType::COMMON_DOWNLOAD, {
        NetType::CELLULAR, NetType::METERED_WIFI, NetType::NOT_METERED_WIFI }, [this](NetType netType) {
            ENGINE_LOGI("DownloadTaskManager OnNetChange netType %{public}d", CAST_INT(netType));
            OnNetChange(netType);
        });
}

void DownloadTaskManager::RestoreDownloadTask()
{
    std::vector<DownloadInfo> downloadInfos;
    DownloadInfoOperator().QueryAll(downloadInfos);
    if (downloadInfos.empty()) {
        ENGINE_LOGI("RestoreDownloadTask, no task to restore");
        return;
    }

    std::lock_guard<ffrt::recursive_mutex> guard(downloadTaskMapMutex_);
    downloadTaskMap_.Clear();

    // 重复任务过滤
    std::map<std::string, DownloadInfo> filter;
    for (const auto &info : downloadInfos) {
        const auto &key = info.taskId + "_" + info.downloadId;
        if (filter.count(key)) {
            ENGINE_LOGE("RestoreDownloadTask, discard duplicated download info, downloadId %{public}s", key.c_str());
            continue;
        }
        filter[key] = info;
    }

    // 任务分组
    std::map<std::string, std::vector<DownloadInfo>> downloadInfoMap;
    for (const auto &[key, downloadInfo] : filter) {
        downloadInfoMap[downloadInfo.taskId].emplace_back(downloadInfo);
    }

    // 创建Task
    for (auto &[taskId, taskDownloadInfos] : downloadInfoMap) {
        auto task = std::make_shared<DownloadTask>(taskId, taskDownloadInfos);
        if (task == nullptr) {
            ENGINE_LOGE("RestoreDownloadTask error, task is nullptr, taskId %{public}s", taskId.c_str());
            continue;
        }
        downloadTaskMap_.EnsureInsert(taskId, task);
        if (IsDownloadComplete(task)) {
            ENGINE_LOGI("IsDownloadComplete true, taskId %{public}s", taskId.c_str());
            continue;
        }
        task->Recover();
    }
}

std::string DownloadTaskManager::GetTaskId(const std::vector<DownloadInfo> &downloadInfos) const
{
    std::vector<std::string> downloadIds;
    for (const auto &downloadInfo : downloadInfos) {
        downloadIds.push_back(downloadInfo.versionId);
    }
    sort(downloadIds.begin(), downloadIds.end());
    std::string srcString;
    for (const auto &downloadId : downloadIds) {
        srcString += downloadId;
    }
    return Sha256Utils().CalculateHashCode(srcString);
}

void DownloadTaskManager::OnNetChange(NetType netType)
{
    ENGINE_LOGD("OnNetChange get mutex start");
    std::lock_guard<ffrt::recursive_mutex> guard(downloadTaskMapMutex_);
    ENGINE_LOGD("OnNetChange get mutex end");
    // 1.从safemap取出内容释放safemap锁  2.执行map每一个元素
    std::map<std::string, std::shared_ptr<DownloadTask>> taskMap;
    auto mapIterate = [&taskMap](const std::string &taskId, const std::shared_ptr<DownloadTask> &task) {
        taskMap[taskId] = task;
    };
    downloadTaskMap_.Iterate(mapIterate);
    ENGINE_LOGD("OnNetChange get mutex completed");

    for (const auto &[taskId, task] : taskMap) {
        if (task == nullptr) {
            continue;
        }
        if (IsDownloadComplete(task)) {
            continue;
        }
        task->OnNetChange(netType);
    }
}

void DownloadTaskManager::DealCallBack(const std::string &taskId, ProgressInfo &progressInfo, DownloadCallback callback)
{
    std::shared_ptr<DownloadTask> taskTmp = nullptr;
    if (!downloadTaskMap_.Find(taskId, taskTmp)) {
        // 任务已经被取消或者完成，输出日志
        ENGINE_LOGE("DealCallBack, task not exist, taskId %{public}s", taskId.c_str());
        return;
    }

    // 下载 成功、失败、取消场景下，在回调中移除task
    std::shared_ptr<DownloadTask> downloadTask = nullptr;
    if (progressInfo.taskStatus == DownloadStatus::FAIL || progressInfo.taskStatus == DownloadStatus::SUCCESS ||
        progressInfo.taskStatus == DownloadStatus::CANCEL) {
        downloadTask = downloadTaskMap_.ReadVal(taskId);
        downloadTaskMap_.Erase(taskId);
    }

    // 回调业务侧
    if (callback != nullptr) {
        callback(taskId, progressInfo);
    }

    if (downloadTask == nullptr) {
        return;
    }

    const auto destroySessionTag = TimeUtils::GetTimestampByMilliseconds();
    // 下载任务在下载失败或者成功之后延迟十秒销毁对象，避免对象提前析构出现问题
    auto delayDestroyTask = [taskId, downloadTask, destroySessionTag]() {
        ENGINE_LOGI("Task download completed, destroy task, tag %{public}" PRId64 " , taskId %{public}s",
            destroySessionTag, taskId.c_str());
    };

    constexpr uint64_t delayUs = 10 * 1000 * 1000; // 10秒的微秒表示
    delayDestroyTaskQueue_.submit(delayDestroyTask, ffrt::task_attr().delay(delayUs));
    ENGINE_LOGI("Task download completed, submit delay destroy task, tag %{public}" PRId64 " , taskId %{public}s",
        destroySessionTag, taskId.c_str());
}

bool DownloadTaskManager::IsDownloadComplete(const std::shared_ptr<DownloadTask> &downloadTask)
{
    DownloadStatus status = downloadTask->GetTaskProgress().taskStatus;
    return status == DownloadStatus::SUCCESS || status == DownloadStatus::CANCEL || status == DownloadStatus::FAIL;
}

bool DownloadTaskManager::GetValidTask(const std::string &taskId, std::shared_ptr<DownloadTask> &outTask)
{
    if (!downloadTaskMap_.Find(taskId, outTask)) {
        ENGINE_LOGE("GetValidTask, task not found, taskId %{public}s", taskId.c_str());
        return false;
    }

    if (outTask == nullptr) {
        ENGINE_LOGE("GetValidTask, task is nullptr, taskId %{public}s", taskId.c_str());
        return false;
    }
    return true;
}
} // namespace OHOS::UpdateService