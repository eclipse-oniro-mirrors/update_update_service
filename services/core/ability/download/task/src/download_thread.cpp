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

#include "download_thread.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <utility>
#include <unistd.h>

#include "anonymous_utils.h"
#include "constant.h"
#include "dupdate_net_manager.h"
#include "file_utils.h"
#include "response_header_parse_utils.h"
#include "sha256_delay_strategy_utils.h"
#include "string_utils.h"
#include "time_utils.h"
#include "update_log.h"

namespace OHOS::UpdateService {
namespace {
constexpr int32_t REQUEST_CONDITION_WAIT_TIME_MILLS = 50;
}

ffrt::recursive_mutex DownloadThread::ffrtInitPackageParamMutex_;

DownloadThread::DownloadThread(const DownloadInfo &downloadInfo, OnDownloadCallback onDownloadCallback)
{
    downloadInfo_ = downloadInfo;
    onDownloadCallback_ = std::move(onDownloadCallback);

    // 暂停恢复场景下，需要校正已下载文件大小
    int64_t fileRealSize = FileUtils::GetFileSize(downloadInfo.path);
    fileInitSize_ = fileRealSize;
    downloadedSize_ = std::max(fileRealSize - CalculatedRollBackSize(fileRealSize), (int64_t)0);
    verifiedSize_ = downloadInfo.verifiedSize;
    totalSize_ = downloadInfo.packageSize;
    downloadInfo_.downloadedSize = downloadedSize_;

    lastEffectiveTime_ = downloadInfo.effectiveTime;
    status_ = downloadInfo.status;

    // 初始化callback数据
    callbackInfo_.downloadedSize = downloadedSize_;
    callbackInfo_.verifiedSize = verifiedSize_;
    callbackInfo_.startTime = downloadInfo.startTime;
    if (downloadInfo.reportInterval > 0) {
        prevCallbackTime_ = TimeUtils::GetTimestampByMilliseconds();
    }

    ENGINE_LOGI("DownloadThread, downloadId %{public}s, packageSize %{public}" PRId64
        ", downloadedSize %{public}" PRId64 ", fileRealSize %{public}" PRId64 "",
        downloadInfo.downloadId.c_str(), downloadInfo_.packageSize, downloadInfo_.downloadedSize, fileRealSize);
}

DownloadThread::~DownloadThread()
{
    ENGINE_LOGI("DownloadThread destructor, downloadId %{public}s", downloadInfo_.downloadId.c_str());
}

void DownloadThread::Start(bool isAsync)
{
    const auto &task = [sharedFromThis = shared_from_this(), this]() {
        if (!ffrtDownloadThreadMutex_.try_lock()) {
            ENGINE_LOGE("Start abort, try lock fail, downloading");
            return;
        }

        // 标记下载线程启动, 用于识别取消下载时是否需要等待Curl结束
        isStarted_ = true;

        DownloadVersion();
        ffrtDownloadThreadMutex_.unlock();
    };

    if (isAsync) {
        ENGINE_LOGD("Start async download thread, downloadId %{public}s", downloadInfo_.downloadId.c_str());
        ffrt::submit(task);
        return;
    }

    ENGINE_LOGD("Start sync download thread, downloadId %{public}s", downloadInfo_.downloadId.c_str());
    task();
}

DownloadResult DownloadThread::Stop()
{
    ENGINE_LOGI("DownloadThread Stop, downloadId %{public}s", downloadInfo_.downloadId.c_str());
    isNeedCancel_ = true;
    StopRequestAndSha256Calculate();

    // 下载已经启动后，需要等待Curl下载结束
    if (isStarted_) {
        std::unique_lock<std::mutex> uniqueLock(requestConditionMutex_);
        requestCondition_.wait_for(uniqueLock, std::chrono::milliseconds(REQUEST_CONDITION_WAIT_TIME_MILLS),
            [this]() { return isRequestFinished_; });
    }

    ENGINE_LOGI(
        "DownloadThread Stop success, downloadId %{public}s, request finished %{public}s, download started %{public}s",
        downloadInfo_.downloadId.c_str(), StringUtils::GetBoolStr(isRequestFinished_).c_str(),
        StringUtils::GetBoolStr(isStarted_).c_str());
    return DownloadResult::SUCCESS;
}

DownloadResult DownloadThread::NetChangeToPause()
{
    ENGINE_LOGI("DownloadThread NetChangeToPause");
    if (status_ == DownloadStatus::INIT) {
        ENGINE_LOGI("thread is not started, callback pause status");
        CallbackDownload(DownloadStatus::AUTO_PAUSE, DownloadEndReason::NET_CHANGE);
        return DownloadResult::SUCCESS;
    }
    if (status_ != DownloadStatus::DOWNLOADING) {
        ENGINE_LOGI("NetChange to pause download, status not downloading");
        return DownloadResult::FAIL;
    }
    isNeedAutoPause_ = true;
    request_.SetDownloadStopStatus(true);
    return DownloadResult::SUCCESS;
}

void DownloadThread::SetDownloadCallback(OnDownloadCallback onDownloadCallback)
{
    onDownloadCallback_ = std::move(onDownloadCallback);
}

void DownloadThread::DownloadVersion()
{
    ENGINE_LOGI("DownloadVersion, download status %{public}d, downloadId %{public}s, package size %{public}" PRId64
        ", downloaded size %{public}" PRId64 "",
        CAST_INT(status_), downloadInfo_.downloadId.c_str(), downloadInfo_.packageSize, downloadInfo_.downloadedSize);
    if (!CheckBeforeStartDownload()) {
        return;
    }

    status_ = DownloadStatus::DOWNLOADING;
    percent_ = 0;
    CallbackDownload(DownloadStatus::DOWNLOADING);
    std::string errorMsg;
    if (InitPackageParam(downloadInfo_.path.c_str(), errorMsg) != DownloadResult::SUCCESS) {
        ENGINE_LOGE("InitPackageParam fail");
        DownloadErrorInfo downloadErrorInfo = BuildDownloadErrorInfo(errorMsg);
        CallbackDownload(DownloadStatus::FAIL, DownloadEndReason::INIT_PACKAGE_FAIL, downloadErrorInfo);
        return;
    }
    DoDownload();
}

void DownloadThread::DoDownload()
{
    downloadStartTime_ = TimeUtils::GetTimestampByMilliseconds();
    isRequestFinished_ = false;
    auto weakThis = weak_from_this();
    DownloadRequestCallback callback{ [weakThis, this](int64_t dlTotal, int64_t dlNow) {
        const auto sharedPtr = weakThis.lock();
        if (sharedPtr == nullptr) {
            ENGINE_LOGE("DownloadVersion, callback is expired");
            return;
        }
        if (!IsPackageSizeAbnormal(dlTotal, dlNow)) {
            isNeedCancel_ = true;
            request_.SetDownloadStopStatus(true);
            HandleSizeError(dlTotal, DownloadEndReason::SIZE_ERROR);
            return;
        };
        CallbackProgress(dlTotal, dlNow);
    } };
    NetworkResponse response = request_.SetUrl(downloadInfo_.url)
                                   .SetHeader(downloadInfo_.header)
                                   .SetRequestBody(downloadInfo_.requestBody)
                                   .SetMethod(downloadInfo_.method)
                                   .SetFile(filePtr_)
                                   .SetDownloadedSize(downloadedSize_)
                                   .SetCallback(callback)
                                   .DoNetRequest();
    SyncFile();
    if (NotifyCancel()) {
        ENGINE_LOGI("DownloadVersion end, downloadId %{public}s", downloadInfo_.downloadId.c_str());
        return;
    }
    HandleDownloadFinish(response.result, response.status);
    isRequestFinished_ = true;
}

void DownloadThread::CallbackDownload(DownloadStatus status, DownloadEndReason endReason,
    const DownloadErrorInfo &downloadErrorInfo)
{
    if (onDownloadCallback_ == nullptr) {
        ENGINE_LOGE("CallbackDownload is nullptr");
        return;
    }

    if (downloadInfo_.reportInterval > 0) {
        prevCallbackTime_ = TimeUtils::GetTimestampByMilliseconds();
    }

    callbackInfo_.downloadId = downloadInfo_.downloadId;
    if (callbackInfo_.startTime == 0) {
        callbackInfo_.startTime = TimeUtils::GetTimestampByMilliseconds();
    }
    if (status == DownloadStatus::SUCCESS || status == DownloadStatus::FAIL) {
        callbackInfo_.endTime = TimeUtils::GetTimestampByMilliseconds();
    }
    callbackInfo_.status = status;
    callbackInfo_.endReason = endReason;
    callbackInfo_.downloadedSize = downloadedSize_;
    callbackInfo_.verifiedSize = verifiedSize_;

    // 下载失败后，已经下载的大小和校验总大小都置为包大小
    if (status == DownloadStatus::FAIL) {
        callbackInfo_.verifiedSize = totalSize_;
        callbackInfo_.downloadedSize = totalSize_;
    }

    // 暂停后恢复下载会回退下载size, 此处避免进度出现回退
    if (downloadedSize_ < fileInitSize_) {
        callbackInfo_.downloadedSize = fileInitSize_;
    }

    // 组装下载异常维测信息
    callbackInfo_.downloadErrorInfo.errorInfo = downloadErrorInfo.errorInfo;
    callbackInfo_.downloadErrorInfo.curlErrorCode = downloadErrorInfo.curlErrorCode;
    callbackInfo_.downloadErrorInfo.httpErrorCode = downloadErrorInfo.httpErrorCode;
    callbackInfo_.downloadErrorInfo.header = downloadErrorInfo.header;

    onDownloadCallback_(callbackInfo_);
}

DownloadResult DownloadThread::InitPackageParam(const char *fileName, std::string &errorMsg)
{
    std::lock_guard<ffrt::recursive_mutex> guard(ffrtInitPackageParamMutex_);

    // fileName的路径为a/b/c.zip,需要创建a/b目录，因此创建多层目录的入参要进行处理
    if (!FileUtils::CreateMultiDirWithPermission(FileUtils::GetParentDir(fileName), FOLDER_PERMISSION)) {
        ENGINE_LOGE("CreateMultiDirWithPermission fail");
        errorMsg = "CreateMultiDirWithPermission fail";
        return DownloadResult::FAIL;
    }

    if (strstr(fileName, "/.") != nullptr || strstr(fileName, "./") != nullptr) {
        ENGINE_LOGE("fileName %{public}s is invalid", AnonymousUtils::GetPrintFilePathStr(fileName).c_str());
        errorMsg.append(fileName).append("is invalid");
        return DownloadResult::FAIL;
    }

    constexpr int32_t storedFilePermission = 0660;
    OpenMode mode = FileUtils::IsFileExist(fileName) ? OpenMode::READ_WRITE : OpenMode::READ_WRITE_CREATE;
    filePtr_ = FileUtils::CreateFileSharedPtr(fileName, mode, storedFilePermission);
    if (filePtr_ == nullptr) {
        ENGINE_LOGE("open file %{public}s failed", AnonymousUtils::GetPrintFilePathStr(fileName).c_str());
        std::string errorStr = std::strerror(errno);
        errorMsg.append("errno=").append(std::to_string(errno)).append(";");
        errorMsg.append("errnoMsg=").append(errorStr);
        return DownloadResult::FAIL;
    }

    int32_t ret = fseek(filePtr_.get(), downloadedSize_, SEEK_SET);
    if (ret != 0) {
        ENGINE_LOGE("fseek file handle fail, errno %{public}d", errno);
        std::string errorStr = std::strerror(errno);
        errorMsg.append("errno=").append(std::to_string(errno)).append(";");
        errorMsg.append("errnoMsg=").append(errorStr).append(";");
        errorMsg.append("fseekRet=").append(std::to_string(ret));
        filePtr_ = nullptr;
        return DownloadResult::FAIL;
    }
    return DownloadResult::SUCCESS;
}

void DownloadThread::HandleDownloadFinish(int curlRet, long responseCode)
{
    ENGINE_LOGI("HandleDownloadFinish, downloadId %{public}s, curlRet %{public}d, response %{public}ld "
        "isNeedAutoPause_ %{public}s",
        downloadInfo_.downloadId.c_str(), curlRet, responseCode, StringUtils::GetBoolStr(isNeedAutoPause_).c_str());
    callbackInfo_.effectiveTime = TimeUtils::GetTimestampByMilliseconds() - downloadStartTime_ + lastEffectiveTime_;
    if (isNeedAutoPause_) {
        isNeedAutoPause_ = false;
        CallbackDownload(DownloadStatus::AUTO_PAUSE, DownloadEndReason::NET_CHANGE);
        return;
    }
    if (curlRet == CURLE_OK || downloadedSize_ == downloadInfo_.packageSize) {
        if (downloadedSize_ != downloadInfo_.packageSize) {
            HandleSizeError(downloadedSize_, DownloadEndReason::FINISH_SIZE_ERROR);
            return;
        }
        DoVerify();
        return;
    }
    HandleFail(curlRet, responseCode);
}

void DownloadThread::DoVerify()
{
    status_ = DownloadStatus::VERIFYING;
    verifiedPercent_ = 0;
    CallbackDownload(DownloadStatus::VERIFYING);

    int64_t fileRealSize = FileUtils::GetFileSize(downloadInfo_.path);
    ENGINE_LOGI("HandleDownloadFinish, fileRealSize: %{public}" PRId64 ", downloadedSize: %{public}" PRId64 "",
        fileRealSize, downloadedSize_);
    OnFileVerifyCallback verifyCallback = nullptr;
    if (downloadInfo_.isCombineVerifyProgress) {
        verifyCallback = [weakThis = weak_from_this(), fileRealSize](int64_t verifiedSize) {
            const auto sharedPtr = weakThis.lock();
            if (sharedPtr == nullptr) {
                ENGINE_LOGE("DoVerify, callback is expired");
                return;
            }
            sharedPtr->CallbackVerifyProgress(fileRealSize, verifiedSize);
        };
    }
    Sha256VerifyResult sha256ResultInfo =
        sha256Utils_.GetFileSha256VerifyResult(downloadInfo_.path, downloadInfo_.verifyInfo, verifyCallback);
    if (NotifyCancel()) {
        ENGINE_LOGI("HandleDownloadFinish, sha256 verify cancel, downloadId %{public}s",
            downloadInfo_.downloadId.c_str());
        return;
    }

    if (sha256ResultInfo.sha256ErrCode == Sha256ErrCode::SUCCESS) {
        ENGINE_LOGI("HandleDownloadFinish, sha256 verify success, downloadId %{public}s",
            downloadInfo_.downloadId.c_str());
        DownloadErrorInfo downloadErrorInfo = BuildDownloadErrorInfo("", request_.GetHeadersBuffer());
        verifiedSize_ = fileRealSize;
        CallbackDownload(DownloadStatus::SUCCESS, DownloadEndReason::SUCCESS, downloadErrorInfo);
        return;
    }

    int64_t fileSize = FileUtils::GetFileSize(downloadInfo_.path);
    ENGINE_LOGE("HandleDownloadFinish, sha256 verify fail, downloadId %{public}s, file size %{public}" PRId64
        ", package size %{public}" PRId64 "",
        downloadInfo_.downloadId.c_str(), fileSize, downloadInfo_.packageSize);
    Sha256FailedReasonInfo sha256FailedReason;
    sha256FailedReason.errorCode = static_cast<int32_t>(sha256ResultInfo.sha256ErrCode);
    sha256FailedReason.sha256Result = sha256ResultInfo.sha256Result;
    sha256FailedReason.fileSize = fileSize;
    DownloadErrorInfo downloadErrorInfo =
        BuildDownloadErrorInfo(sha256FailedReason.ToString(), request_.GetHeadersBuffer());
    CallbackDownload(DownloadStatus::FAIL, DownloadEndReason::VERIFY_FAIL, downloadErrorInfo);
}

void DownloadThread::CallbackVerifyProgress(int64_t fileSize, int64_t verifiedSize)
{
    if (isNeedCancel_) {
        return;
    }
    if (fileSize <= 0 || verifiedSize > fileSize) {
        ENGINE_LOGE("CallbackVerifyProgress, verifiedSize error, verifiedSize: %{public}" PRId64 "", verifiedSize);
        return;
    }
    // 防止进度回退
    if (verifiedSize <= verifiedSize_) {
        return;
    }
    verifiedSize_ = verifiedSize;
    // 降低回调频率, 进度为10的倍数才回调
    constexpr int32_t callbackThreshold = 10;
    if (fileSize == 0) {
        ENGINE_LOGE("CallbackVerifyProgress, fileSize is 0, cannot calculate percent");
        return;
    }
    auto verifiedPercent = static_cast<int32_t>(verifiedSize * DOWNLOAD_MAX_PERCENT / fileSize);
    if (verifiedPercent > verifiedPercent_ && verifiedPercent % callbackThreshold == 0) {
        ENGINE_LOGI("CallbackVerifyProgress, verifiedPercent = %{public}d, verifiedSize = %{public}" PRId64 ", "
            "fileSize = %{public}" PRId64 "",
            verifiedPercent, verifiedSize, fileSize);
        status_ = DownloadStatus::VERIFYING;
        verifiedPercent_ = verifiedPercent;
        CallbackDownload(DownloadStatus::VERIFYING);
    }
}

void DownloadThread::HandleFail(int curlRet, long responseCode)
{
    if (curlRet == CURLE_WRITE_ERROR || curlRet == CURLE_READ_ERROR) {
        ENGINE_LOGE("HandleFail, caused by io exception, downloadId %{public}s", downloadInfo_.downloadId.c_str());
        std::string errorInfo;
        std::string errorMsg = std::strerror(errno);
        errorInfo.append("errno=").append(std::to_string(errno)).append(";");
        errorInfo.append("errnoMsg=").append(errorMsg).append(";");
        DownloadErrorInfo downloadErrorInfo =
            BuildDownloadErrorInfo(errorInfo, request_.GetHeadersBuffer(), curlRet, responseCode);
        CallbackDownload(DownloadStatus::FAIL, DownloadEndReason::IO_EXCEPTION, downloadErrorInfo);
        return;
    }
    if (responseCode == HttpRespondCode::CODE_503 || responseCode == HttpRespondCode::CODE_504) {
        ENGINE_LOGE("HandleFail, caused by server timeout, downloadId %{public}s", downloadInfo_.downloadId.c_str());
        std::string errorInfo;
        DownloadErrorInfo downloadErrorInfo =
            BuildDownloadErrorInfo(errorInfo, request_.GetHeadersBuffer(), curlRet, responseCode);
        CallbackDownload(DownloadStatus::FAIL, DownloadEndReason::SERVER_TIMEOUT, downloadErrorInfo);
        return;
    }
    if (responseCode == HttpRespondCode::CODE_301 || responseCode == HttpRespondCode::CODE_302 ||
        responseCode == HttpRespondCode::CODE_303 || responseCode == HttpRespondCode::CODE_307) {
        ENGINE_LOGE("HandleFail, caused by redirect, downloadId %{public}s", downloadInfo_.downloadId.c_str());
        std::string errorInfo;
        DownloadErrorInfo downloadErrorInfo =
            BuildDownloadErrorInfo(errorInfo, request_.GetHeadersBuffer(), curlRet, responseCode);
        CallbackDownload(DownloadStatus::FAIL, DownloadEndReason::REDIRECT, downloadErrorInfo);
        return;
    }
    HandleNetworkCheck(curlRet, responseCode);
}

void DownloadThread::HandleNetworkCheck(int curlRet, long responseCode)
{
    DownloadErrorInfo dlErrorInfo = BuildDownloadErrorInfo("", request_.GetHeadersBuffer(), curlRet, responseCode);
    if (!NetManager::GetInstance()->IsNetAvailable()) {
        ENGINE_LOGI("HandleNetworkCheck net is not available callback NET_NOT_AVAILIABLE");
        CallbackDownload(DownloadStatus::AUTO_PAUSE, DownloadEndReason::NET_NOT_AVAILIABLE, dlErrorInfo);
        return;
    }
    if (isNeedAutoPause_) {
        ENGINE_LOGI("HandleNetworkCheck isNeedAutoPause_ is true, callback NET_CHANGE");
        isNeedAutoPause_ = false;
        CallbackDownload(DownloadStatus::AUTO_PAUSE, DownloadEndReason::NET_CHANGE, dlErrorInfo);
        return;
    }
    ENGINE_LOGI("HandleNetworkCheck callback CURL_ERROR");
    CallbackDownload(DownloadStatus::AUTO_PAUSE, DownloadEndReason::CURL_ERROR, dlErrorInfo);
}

/* 用于返回下载百分比回调 */
void DownloadThread::CallbackProgress(int64_t dlTotal, int64_t dlNow)
{
    // 设置状态重置标记，用于通知service重置重试次数；当从网络下载到数据超过初始文件大小时，允许重置
    if (!callbackInfo_.resetRetryTimes && fileInitSize_ != 0 && dlNow > fileInitSize_) {
        ENGINE_LOGI("Reset retryTimes, downloadId %{public}s, downloadedSize %{public}" PRId64
            ", initial file size %{public}" PRId64 "",
            downloadInfo_.downloadId.c_str(), downloadedSize_, fileInitSize_);
        callbackInfo_.resetRetryTimes = true;
    }

    downloadedSize_ = dlNow;
    if (isDisableCallbackFlag_) {
        return;
    }

    int32_t outputProgress = (downloadInfo_.packageSize == 0) ?
        0 :
        static_cast<int32_t>(downloadedSize_ * DOWNLOAD_MAX_PERCENT / downloadInfo_.packageSize);
    if ((outputProgress > percent_) && (outputProgress <= DOWNLOAD_MAX_PERCENT)) {
        percent_ = outputProgress;
        status_ = DownloadStatus::DOWNLOADING;
        ENGINE_LOGI("outputProgress = %{public}d, dlNow =%{public}" PRId64 ", dlTotal = %{public}" PRId64 ","
            "downloadedSize = %{public}" PRId64 "",
            outputProgress, (int64_t)dlNow, (int64_t)dlTotal, downloadedSize_);
        if (outputProgress == DOWNLOAD_MAX_PERCENT) {
            bool isScreenOnAndOUCNotDisplay =
                Sha256DelayStrategyManager::GetInstance()->IsScreenOnAndOUCNotDisplay(downloadInfo_.sha256StrategyKey);
            Sha256DelayScene scene = isScreenOnAndOUCNotDisplay ? Sha256DelayScene::SCREEN_ON_AND_OUC_NOT_DISPLAY :
                                                                  Sha256DelayScene::SCREEN_OFF_OR_OUC_DISPLAY;
            sha256Utils_.SetDelayStatus(scene);
        }
        callbackInfo_.effectiveTime = TimeUtils::GetTimestampByMilliseconds() - downloadStartTime_ + lastEffectiveTime_;
        CallbackDownload(DownloadStatus::DOWNLOADING);
        return;
    }

    if (downloadInfo_.reportInterval <= 0) {
        return;
    }

    const auto currTime = TimeUtils::GetTimestampByMilliseconds();
    if ((currTime - prevCallbackTime_) > downloadInfo_.reportInterval) {
        CallbackDownload(DownloadStatus::DOWNLOADING);
    }
}

void DownloadThread::SetDisableCallbackFlag(bool flag)
{
    isDisableCallbackFlag_ = flag;
}

bool DownloadThread::NotifyCancel()
{
    // 如果取消/暂停下载，直接在对应的方法通过通知后同步回调
    if (isNeedCancel_) {
        isRequestFinished_ = true;
        std::unique_lock<std::mutex> uniqueLock(requestConditionMutex_);
        requestCondition_.notify_all();
        ENGINE_LOGI("NotifyCancel end");
        return true;
    }
    return false;
}

void DownloadThread::StopRequestAndSha256Calculate()
{
    request_.SetDownloadStopStatus(true);
}

bool DownloadThread::CheckBeforeStartDownload()
{
    if (downloadInfo_.IsCompleted()) {
        ENGINE_LOGE("CheckBeforeStartDownload abort, download completed, status %{public}d, downloadId %{public}s",
            CAST_INT(downloadInfo_.status), downloadInfo_.downloadId.c_str());
        CallbackDownload(downloadInfo_.status);
        return false;
    }

    if (downloadInfo_.IsNeedVerify()) {
        // 执行校验
        ENGINE_LOGI("CheckBeforeStartDownload, DowmloadInfo need verify, downloadId %{public}s",
            downloadInfo_.downloadId.c_str());
        DoVerify();
        return false;
    }
    return true;
}

bool DownloadThread::IsPackageSizeAbnormal(int64_t dlTotal, int64_t dlNow)
{
    if (isPackageSizeVerified_ || dlTotal == dlNow) {
        return true;
    }
    isPackageSizeVerified_ = true;
    if (downloadInfo_.packageSize == 0) {
        ENGINE_LOGE("IsPackageSizeAbnormal, package size is abnormal, downloadId %{public}s",
            downloadInfo_.downloadId.c_str());
        return false;
    }

    if (IsContentLengtAbnormal(dlTotal)) {
        return true;
    }

    if (downloadInfo_.packageSize != dlTotal) {
        ENGINE_LOGE("IsPackageSizeAbnormal, package size verify fail, downloadId %{public}s, dlTotal: %{public}" PRId64
            "packageSize: %{public}" PRId64,
            downloadInfo_.downloadId.c_str(), dlTotal, downloadInfo_.packageSize);
        return false;
    }
    return true;
}

bool DownloadThread::IsContentLengtAbnormal(int64_t dlTotal)
{
    if (ResponseHeaderParseUtils::FindKeyFromHeader(request_.GetHeadersBuffer(), "Transfer-Encoding")) {
        ENGINE_LOGE("header contain Transfer-Encoding");
        return true;
    }
    int64_t contentDownloadSize = GetResponseSize() + downloadedSize_;
    if (contentDownloadSize != dlTotal) {
        ENGINE_LOGE(
            "IsContentLengtAbnormal, package size verify fail, downloadId %{public}s, dlTotal = %{public}" PRId64 ", "
            "contentDownloadSize = %{public}" PRId64,
            downloadInfo_.downloadId.c_str(), dlTotal, contentDownloadSize);
        return false;
    }

    if (contentDownloadSize != downloadInfo_.packageSize) {
        ENGINE_LOGE("IsContentLengtAbnormal, package size verify fail, downloadId %{public}s, contentDownloadSize = "
            "%{public}" PRId64 ", packageSize = %{public}" PRId64,
            downloadInfo_.downloadId.c_str(), contentDownloadSize, downloadInfo_.packageSize);
        return false;
    }
    return true;
}

int64_t DownloadThread::GetResponseSize()
{
    std::string contentLengthVulue =
        ResponseHeaderParseUtils::ParseKeyFromHeader(request_.GetHeadersBuffer(), "Content-Length");
    int64_t contentLengthNum = 0;
    if (StringUtils::DecStringToNumber(contentLengthVulue, contentLengthNum) != StrCnvResult::SUCCESS) {
        ENGINE_LOGE("DecStringToNumber fail %{public}s", contentLengthVulue.c_str());
        return 0;
    };
    return contentLengthNum;
}

int64_t DownloadThread::CalculatedRollBackSize(int64_t fileDownloadSize)
{
    ENGINE_LOGI("DownloadThread fileDownloadSize %{public}" PRId64 "", fileDownloadSize);
    if (fileDownloadSize <= 0) {
        return 0;
    }
    return fileDownloadSize <= Constant::MIN_FILE_DOWNLOAD_SIZE ? fileDownloadSize : Constant::MIN_FILE_DOWNLOAD_SIZE;
}

void DownloadThread::SyncFile()
{
    FILE *file = filePtr_.get();
    int32_t ret = fflush(file);
    if (ret != 0) {
        ENGINE_LOGE("flush file fail, errno %{public}d", errno);
    }
    ret = fsync(fileno(file));
    if (ret != 0) {
        ENGINE_LOGE("fsync file fail, errno %{public}d", errno);
    }
    filePtr_ = nullptr;
}

DownloadErrorInfo DownloadThread::BuildDownloadErrorInfo(const std::string &errorInfo,
    const std::vector<std::string> &headers, const int32_t curlErrorCode, const int64_t httpErrorCode)
{
    DownloadErrorInfo downloadErrorInfo;
    downloadErrorInfo.curlErrorCode = curlErrorCode;
    downloadErrorInfo.httpErrorCode = httpErrorCode;
    downloadErrorInfo.header = headers;
    downloadErrorInfo.errorInfo = errorInfo;
    return downloadErrorInfo;
}

void DownloadThread::HandleSizeError(int64_t requestSize, const DownloadEndReason endReason)
{
    DownloadSizeErrorInfo errorInfo;
    errorInfo.requestSize = requestSize;
    errorInfo.filelistPackageSize = downloadInfo_.packageSize;
    DownloadErrorInfo downloadErrorInfo = BuildDownloadErrorInfo(errorInfo.ToString(), request_.GetHeadersBuffer());
    CallbackDownload(DownloadStatus::FAIL, endReason, downloadErrorInfo);
}
} // namespace OHOS::UpdateService
