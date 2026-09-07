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

#include "firmware_download_executor.h"

#include "ffrt.h"

#include "download_task_manager.h"
#include "dupdate_errno.h"
#include "firmware_callback_utils.h"
#include "firmware_component_operator.h"
#include "firmware_log.h"
#include "firmware_task_operator.h"
#include "firmware_update_helper.h"
#include "string_utils.h"

namespace OHOS {
namespace UpdateService {
void FirmwareDownloadExecutor::Execute()
{
    FIRMWARE_LOGI("FirmwareDownloadExecutor::Execute");
    ffrt::submit([ex = shared_from_this()] { ex->DoDownload(); });
}

void FirmwareDownloadExecutor::DoDownload()
{
    FirmwareComponentOperator().QueryAll(components_);
    FIRMWARE_LOGI("Execute size %{public}d", CAST_INT(components_.size()));
    if (components_.size() == 0) {
        Progress progress;
        progress.status = UpgradeStatus::DOWNLOAD_FAIL;
        progress.endReason = "no task";
        firmwareProgressCallback_.progressCallback(progress);
        return;
    }

    GetTask();
    if (tasks_.downloadTaskId.empty()) {
        // 首次触发下载
        PerformDownload();
        return;
    }

    // 恢复下载
    PerformResumeDownload();
}

void FirmwareDownloadExecutor::PerformDownload()
{
    std::vector<DownloadInfo> downloadInfos = BuildDownloadInfos();
    if (downloadInfos.empty()) {
        return;
    }
    auto weakThis = weak_from_this();
    auto cb = [weakThis, this](const std::string &taskId, ProgressInfo progressInfo) {
        const auto sharedPtr = weakThis.lock();
        if (sharedPtr == nullptr) {
            FIRMWARE_LOGE("PerformDownload, callback is expired, taskId %{public}s, %{public}s", taskId.c_str(),
                progressInfo.ToString().c_str());
            return;
        }
        CallbackProgress(taskId, progressInfo);
    };

    std::string downloadTaskId = DownloadTaskManager::GetInstance()->GetTaskId(downloadInfos);

    if (downloadCallback_.eventCallback != nullptr) {
        downloadCallback_.eventCallback(downloadTaskId, EventId::EVENT_DOWNLOAD_START);
    }

    DownloadError downloadError;
    downloadTaskId =
        DownloadTaskManager::GetInstance()->Start(downloadInfos, cb, downloadError, DownloadDirType::UN_ENCRYPT_DIR);

    FIRMWARE_LOGE("PerformDownload downloadTaskId %{public}s downloadError: %{public}d", downloadTaskId.c_str(),
        CAST_INT(downloadError.endReason));

    if (downloadError.errorNum != DownloadCallResult::SUCCESS) {
        CallbackDownloadFailProgress(downloadError);
    }
}

void FirmwareDownloadExecutor::PerformResumeDownload()
{
    DownloadError downloadError;
    ProgressInfo progressInfo =
        DownloadTaskManager::GetInstance()->GetTaskProgress(tasks_.downloadTaskId, downloadError);

    FIRMWARE_LOGI("PerformResumeDownload progressInfo taskStatus %{public}d resumeDownloadError errorNum %{public}d",
        CAST_INT(progressInfo.taskStatus), CAST_INT(downloadError.errorNum));
    if (downloadError.errorNum != DownloadCallResult::SUCCESS) {
        if (downloadError.endReason == DownloadEndReason::DOWNLOAD_INFO_EMPTY) {
            PerformDownload();
            return;
        }
        CallbackDownloadFailProgress(downloadError);
        return;
    }

    if (!IsNeedResumeDownload(progressInfo, downloadError)) {
        return;
    }

    auto weakThis = weak_from_this();
    DownloadCallback cb = [weakThis, this](const std::string &taskId, ProgressInfo progressInfo) {
        const auto sharedPtr = weakThis.lock();
        if (sharedPtr == nullptr) {
            FIRMWARE_LOGE("PerformResumeDownload, callback is expired, taskId %{public}s, %{public}s", taskId.c_str(),
                progressInfo.ToString().c_str());
            return;
        }
        CallbackProgress(taskId, progressInfo);
    };

    if (downloadCallback_.eventCallback != nullptr) {
        downloadCallback_.eventCallback(tasks_.downloadTaskId, EventId::EVENT_DOWNLOAD_RESUME);
    }
    DownloadTaskManager::GetInstance()->Resume(tasks_.downloadTaskId, downloadOptions_.allowNetwork, cb, downloadError);
    if (downloadError.errorNum == DownloadCallResult::SUCCESS) {
        FIRMWARE_LOGI("PerformResumeDownload success, taskId %{public}s", tasks_.downloadTaskId.c_str());
        return;
    }

    FIRMWARE_LOGI("PerformResumeDownload fail, taskId %{public}s, reason %{public}d", tasks_.downloadTaskId.c_str(),
        CAST_INT(downloadError.endReason));
    if (downloadError.endReason == DownloadEndReason::NET_NOT_AVAILIABLE) {
        progressInfo.reason = DownloadEndReason::NET_NOT_AVAILIABLE;
        CallbackProgress(tasks_.downloadTaskId, progressInfo);
        return;
    }
    CallbackDownloadFailProgress(downloadError);
}

void FirmwareDownloadExecutor::GetTask()
{
    FirmwareTaskOperator().QueryTask(tasks_);
}

bool FirmwareDownloadExecutor::VerifyDownloadPkg(const std::string &pkgName, Progress &progress)
{
    std::string verifyInfo = "";
    for (const auto &component : components_) {
        if (component.spath == pkgName) {
            verifyInfo = component.verifyInfo;
            break;
        }
    }
    ENGINE_LOGI("Start Checking file Sha256 %{public}s, verifyInfo %{public}s", pkgName.c_str(), verifyInfo.c_str());
    if (!verifyInfo.empty() && !Sha256Utils::CheckFileSha256String(pkgName, verifyInfo)) {
        ENGINE_LOGE("file sha256 check error, fileName:%{public}s", pkgName.c_str());
        return false;
    }
    return true;
}

std::vector<DownloadInfo> FirmwareDownloadExecutor::BuildDownloadInfos() const
{
    std::vector<DownloadInfo> downLoadInfos;
    for (const auto &component : components_) {
        DownloadInfo downloadInfo;
        downloadInfo.versionId = component.versionId;
        downloadInfo.url = component.url;
        downloadInfo.path = component.spath;
        downloadInfo.packageSize = component.size;
        downloadInfo.verifyInfo = component.verifyInfo;
        downloadInfo.isNeedAutoResume = false;
        downloadInfo.method = RequestMethod::GET;
        downloadInfo.netType = downloadOptions_.allowNetwork;
        downLoadInfos.push_back(downloadInfo);
    }
    return downLoadInfos;
}

FirmwareDownloadProgress FirmwareDownloadExecutor::BuildFirmwareDownloadProgress(const Progress &progress,
    const DownloadProgress &downloadProgress)
{
    FirmwareDownloadProgress firmwareDownloadProgress;
    firmwareDownloadProgress.versionId = downloadProgress.id;
    firmwareDownloadProgress.downloadUrl = downloadProgress.downloadUrl;
    firmwareDownloadProgress.startTime = downloadProgress.startTime;
    firmwareDownloadProgress.endTime = downloadProgress.endTime;
    firmwareDownloadProgress.effectiveTime = downloadProgress.effectiveTime;
    firmwareDownloadProgress.progress = progress;
    firmwareDownloadProgress.downloadErrorInfo = downloadProgress.downloadErrorInfo;
    firmwareDownloadProgress.packageSize = downloadProgress.packageSize;
    firmwareDownloadProgress.downloadedSize = downloadProgress.downloadedSize;
    firmwareDownloadProgress.verifiedSize = downloadProgress.verifiedSize;
    firmwareDownloadProgress.isCombineVerifyProgress = downloadProgress.isCombineVerifyProgress;
    return firmwareDownloadProgress;
}

void FirmwareDownloadExecutor::CallbackProgress(std::string taskId, ProgressInfo progressInfo)
{
    FIRMWARE_LOGI("downloadCallback taskId = %{public}s, status = %{public}d, progress = %{public}d, endReason = "
        "%{public}d",
        taskId.c_str(), progressInfo.taskStatus, progressInfo.taskProgress, progressInfo.reason);
    static const std::map<DownloadStatus, UpgradeStatus> statusMap = {
        { DownloadStatus::DOWNLOADING, UpgradeStatus::DOWNLOADING },
        { DownloadStatus::PAUSE, UpgradeStatus::DOWNLOAD_PAUSE },
        { DownloadStatus::CANCEL, UpgradeStatus::DOWNLOAD_CANCEL },
        { DownloadStatus::FAIL, UpgradeStatus::DOWNLOAD_FAIL },
        { DownloadStatus::SUCCESS, UpgradeStatus::DOWNLOAD_SUCCESS },
        { DownloadStatus::VERIFYING, UpgradeStatus::VERIFYING },
        { DownloadStatus::AUTO_PAUSE, UpgradeStatus::DOWNLOAD_PAUSE },
        { DownloadStatus::INIT, UpgradeStatus::DOWNLOADING }
    };

    FirmwareDownloadCallbackInfo downloadCallbackInfo;
    for (const DownloadProgress &downloadProgress : progressInfo.progresses) {
        auto iter = statusMap.find(downloadProgress.status);
        if (iter == statusMap.end()) {
            FIRMWARE_LOGE("downloadCallback unknow type %{public}d", downloadProgress.status);
            continue;
        }
        Progress progress;
        progress.status = iter->second;
        progress.percent = downloadProgress.percent;
        progress.endReason = std::to_string(CAST_INT(downloadProgress.reason));
        FirmwareDownloadProgress firmwareDownloadProgress = BuildFirmwareDownloadProgress(progress, downloadProgress);
        downloadCallbackInfo.progressList.push_back(firmwareDownloadProgress);
    }

    auto iter = statusMap.find(progressInfo.taskStatus);
    if (iter == statusMap.end()) {
        FIRMWARE_LOGE("downloadCallback unknow task type %{public}d", progressInfo.taskStatus);
        return;
    }

    if (downloadCallback_.progressCallback == nullptr) {
        return;
    }
    Progress taskProgress;
    taskProgress.percent = CAST_UINT(progressInfo.taskProgress);
    taskProgress.status = iter->second;
    taskProgress.endReason = GetEndReason(iter->second, progressInfo.reason);
    downloadCallbackInfo.taskProgress = taskProgress;
    downloadCallbackInfo.downloadTaskId = taskId;
    downloadCallback_.progressCallback(downloadCallbackInfo);
}

std::string FirmwareDownloadExecutor::GetEndReason(UpgradeStatus status, DownloadEndReason reason)
{
    if (status == UpgradeStatus::DOWNLOAD_PAUSE) {
        if (reason == DownloadEndReason::NET_CHANGE) {
            return std::to_string(DUpdateErrno::DUPDATE_ERR_DLOAD_NET_CHANGED);
        }
        if (reason == DownloadEndReason::NET_NOT_AVAILIABLE) {
            return std::to_string(DUpdateErrno::DUPDATE_ERR_DLOAD_REQUEST_FAIL);
        }
    }

    // 下载完成，错误原因返回给hap
    if (status == UpgradeStatus::DOWNLOAD_FAIL) {
        if (reason == DownloadEndReason::VERIFY_FAIL) {
            return std::to_string(DUpdateErrno::DUPDATE_ERR_VERIFY_PACKAGE_FAIL);
        }
        if (reason == DownloadEndReason::NO_ENOUGH_MEMORY) {
            return std::to_string(DUpdateErrno::DUPDATE_ERR_NO_ENOUGH_MEMORY);
        }
        if (reason == DownloadEndReason::IO_EXCEPTION) {
            return std::to_string(DUpdateErrno::DUPDATE_ERR_IO_EXCEPTION);
        }
    }
    return "";
}

void FirmwareDownloadExecutor::CallbackDownloadFailProgress(DownloadError error)
{
    FirmwareDownloadCallbackInfo downloadCallbackInfo;
    downloadCallbackInfo.taskProgress.status = UpgradeStatus::DOWNLOAD_FAIL;
    downloadCallbackInfo.taskProgress.endReason = std::to_string(CAST_INT(error.errorNum));
    if (downloadCallback_.progressCallback != nullptr) {
        downloadCallback_.progressCallback(downloadCallbackInfo);
    }
}

bool FirmwareDownloadExecutor::IsNeedResumeDownload(const ProgressInfo &progressInfo, DownloadError &downloadError)
{
    // 如果下载已经结束（成功、失败），则回调一次下载进度，用于刷新 firmware 数据库中下载任务的状态
    if (progressInfo.IsDownloadFinished()) {
        CallbackProgress(tasks_.downloadTaskId, progressInfo);
        return false;
    }

    if (progressInfo.taskStatus != DownloadStatus::PAUSE && progressInfo.taskStatus != DownloadStatus::AUTO_PAUSE) {
        downloadError.errorNum = DownloadCallResult::FAIL;
        CallbackDownloadFailProgress(downloadError);
        return false;
    }
    return true;
}
} // namespace UpdateService
} // namespace OHOS
