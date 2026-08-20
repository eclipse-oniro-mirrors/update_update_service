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

#ifndef DOWNLOAD_THREAD_H
#define DOWNLOAD_THREAD_H

#include <condition_variable>
#include <curl/curl.h>
#include <map>
#include <string>

#include "ffrt.h"

#include "download_common.h"
#include "download_request.h"
#include "sha256_utils.h"

namespace OHOS::UpdateService {
struct DownloadSizeErrorInfo {
    int64_t requestSize = 0;
    int64_t filelistPackageSize = 0;

    [[nodiscard]] std::string ToString() const
    {
        std::string str;
        str.append("realPackageSize=").append(std::to_string(requestSize)).append(";");
        str.append("filelistPackageSize=").append(std::to_string(filelistPackageSize));
        return str;
    }
};

struct Sha256FailedReasonInfo {
    int32_t errorCode = 0;
    std::string sha256Result;
    int64_t fileSize = 0;

    [[nodiscard]] std::string ToString() const
    {
        std::string str;
        str.append("sha256ErrCode=").append(std::to_string(errorCode)).append(";");
        str.append("sha256Result=").append(sha256Result).append(";");
        str.append("DownloadFileSize=").append(std::to_string(fileSize));
        return str;
    }
};

class DownloadThread : public std::enable_shared_from_this<DownloadThread> {
public:
    DownloadThread(const DownloadInfo &downloadInfo, OnDownloadCallback onDownloadCallback);
    ~DownloadThread();

    void Start(bool isAsync = true);
    DownloadResult Stop();
    DownloadResult NetChangeToPause();

    void SetDownloadCallback(OnDownloadCallback onDownloadCallback);
    void SetDisableCallbackFlag(bool flag);

private:
    DownloadResult InitPackageParam(const char *fileName, std::string &errorMsg);
    void HandleDownloadFinish(int curlRet, long responseCode);
    void HandleFail(int curlRet, long responseCode);
    void CallbackProgress(int64_t dlTotal, int64_t dlNow);
    void CallbackVerifyProgress(int64_t fileSize, int64_t verifiedSize);
    void CallbackDownload(DownloadStatus status, DownloadEndReason endReason = DownloadEndReason::INIT,
        const DownloadErrorInfo &downloadErrorInfo = { static_cast<int32_t>(CURLE_OK),
                                                       static_cast<int64_t>(HttpRespondCode::CODE_SUCCESS),
                                                       {},
                                                       "" });
    void DownloadVersion();
    void HandleNetworkCheck(int curlRet, long responseCode);
    bool NotifyCancel();
    void StopRequestAndSha256Calculate();

    void DoVerify();
    bool CheckBeforeStartDownload();
    bool IsPackageSizeAbnormal(int64_t dlTotal, int64_t dlNow);
    int64_t CalculatedRollBackSize(int64_t fileSize);
    void SyncFile();
    void DoDownload();
    int64_t GetResponseSize();
    bool IsContentLengtAbnormal(int64_t dlTotal);
    DownloadErrorInfo BuildDownloadErrorInfo(const std::string &errorInfo, const std::vector<std::string> &headers = {},
        const int32_t curlErrorCode = static_cast<int32_t>(CURLE_OK),
        const int64_t httpErrorCode = static_cast<int64_t>(HttpRespondCode::CODE_SUCCESS));
    void HandleSizeError(int64_t requestSize, const DownloadEndReason endReason);

private:
    DownloadInfo downloadInfo_;
    DownloadStatus status_ = DownloadStatus::INIT;
    DownloadCallbackInfo callbackInfo_;
    int32_t percent_ = 0;
    int64_t downloadedSize_ = 0;
    int64_t totalSize_ = 0;
    int64_t fileInitSize_ = 0;
    std::shared_ptr<FILE> filePtr_ = nullptr;

    int64_t verifiedSize_ = 0;
    int32_t verifiedPercent_ = 0;

    bool isNeedAutoPause_ = false;
    bool isNeedCancel_ = false;
    bool isPackageSizeVerified_ = false;

    DownloadRequest request_;
    OnDownloadCallback onDownloadCallback_ = nullptr;

    static ffrt::recursive_mutex ffrtInitPackageParamMutex_;

    ffrt::recursive_mutex ffrtDownloadThreadMutex_;

    bool isStarted_ = false;
    bool isRequestFinished_ = false;
    std::mutex requestConditionMutex_;
    std::condition_variable requestCondition_;

    bool isDisableCallbackFlag_ = false;
    Sha256Utils sha256Utils_;

    int64_t downloadStartTime_ = 0;
    int64_t lastEffectiveTime_ = 0;

    int64_t prevCallbackTime_ = 0;
};
} // namespace OHOS::UpdateService
#endif // DOWNLOAD_THREAD_H