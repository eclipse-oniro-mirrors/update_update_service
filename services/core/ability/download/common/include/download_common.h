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

#ifndef DOWNLOAD_COMMON_H
#define DOWNLOAD_COMMON_H

#include <functional>
#include <map>
#include <string>

#include "anonymous_utils.h"
#include "network_response.h"
#include "network_type.h"
#include "update_define.h"

namespace OHOS::UpdateService {
constexpr int32_t DOWNLOAD_MAX_PERCENT = 100;

enum class DownloadEndReason {
    INIT = 0,
    SUCCESS = 1,
    DOWNLOADING = 2,
    FAIL = 3,
    VERIFY_FAIL = 4,
    IO_EXCEPTION = 5,
    REDIRECT = 6,
    SERVER_TIMEOUT = 7,
    PAUSE = 8,
    CANCEL = 9,
    NO_ENOUGH_MEMORY = 10,
    NET_NOT_AVAILIABLE,
    DOWNLOAD_INFO_EMPTY,
    CURL_ERROR,
    SYSTEM_BUSY,
    INIT_PACKAGE_FAIL,
    NET_CHANGE,
    SIZE_ERROR,
    FINISH_SIZE_ERROR
};

enum class DownloadStatus {
    INIT = 0,
    DOWNLOADING = 20,
    PAUSE = 21,
    CANCEL = 22,
    FAIL = 23,
    SUCCESS = 24,
    VERIFYING = 25,
    AUTO_PAUSE = 26,
};

enum class DownloadDirType {
    UN_ENCRYPT_DIR = 0,
    ENCRYPT_DIR = 1,
};

enum class DownloadCallResult {
    SUCCESS = 0,
    FAIL = 1,
};

enum class DownloadResult {
    SUCCESS = 0,
    FAIL = 1,
};

struct DownloadError {
    DownloadEndReason endReason = DownloadEndReason::INIT;
    DownloadCallResult errorNum = DownloadCallResult::SUCCESS;

    DownloadError &build(DownloadEndReason reason, DownloadCallResult result)
    {
        endReason = reason;
        errorNum = result;
        return *this;
    }
};

struct DownloadErrorInfo {
    int32_t curlErrorCode = 0;
    int64_t httpErrorCode = 0;
    std::vector<std::string> header;
    std::string errorInfo;
};

struct DownloadCallbackInfo {
    std::string downloadId;
    int64_t downloadedSize = 0;
    int64_t startTime = 0;
    int64_t endTime = 0;
    int64_t effectiveTime = 0;
    DownloadStatus status = DownloadStatus::DOWNLOADING;
    DownloadEndReason endReason = DownloadEndReason::INIT;
    bool resetRetryTimes = false;
    DownloadErrorInfo downloadErrorInfo;
    int64_t verifiedSize = 0;

    [[nodiscard]] std::string ToString() const
    {
        return std::string("DownloadCallbackInfo: ")
            .append("downloadId=")
            .append(downloadId)
            .append(",")
            .append("downloadedSize=")
            .append(std::to_string(downloadedSize))
            .append(",")
            .append("verifiedSize=")
            .append(std::to_string(verifiedSize))
            .append(",")
            .append("startTime=")
            .append(std::to_string(startTime))
            .append(",")
            .append("endTime=")
            .append(std::to_string(endTime))
            .append(",")
            .append("effectiveTime=")
            .append(std::to_string(effectiveTime))
            .append(",")
            .append("status=")
            .append(std::to_string(CAST_INT(status)))
            .append(",")
            .append("endReason=")
            .append(std::to_string(CAST_INT(endReason)));
    }
};

struct DownloadInfo {
    int32_t id = 0;
    std::string downloadId;
    std::string versionId;
    std::string taskId;
    std::string url;
    std::string reserveUrl;
    std::string path;
    std::string verifyInfo;
    std::map<std::string, std::string> header;
    std::string requestBody;
    std::string sha256StrategyKey;
    DownloadEndReason endReason = DownloadEndReason::INIT;
    DownloadStatus status = DownloadStatus::INIT;
    DownloadDirType dirType = DownloadDirType::UN_ENCRYPT_DIR;
    NetType netType = NetType::NOT_METERED_WIFI;
    RequestMethod method = RequestMethod::GET;
    int32_t retryTimes = 0;
    int64_t packageSize = 0;
    int64_t downloadedSize = 0;
    int64_t startTime = 0;
    int64_t endTime = 0;
    int64_t effectiveTime = 0;
    bool isNeedAutoResume = false;
    DownloadErrorInfo downloadErrorInfo;
    bool isConcurrentLimited = false;
    bool isCombineVerifyProgress = false;
    int64_t verifiedSize = 0;
    bool isIgnoredFail = false; // 是否错误当成功处理
    int32_t reportInterval = 0;

    bool operator < (const DownloadInfo &downloadInfo) const
    {
        if (downloadId < downloadInfo.downloadId)
            return true;
        if (downloadId > downloadInfo.downloadId)
            return false;
        return false;
    }

    [[nodiscard]] std::string ToString() const
    {
        std::string info = "DownloadInfo: ";
        info.append("id=").append(std::to_string(id)).append(",");
        info.append("taskId=").append(taskId).append(",");
        info.append("url=").append(AnonymousUtils::AnonymousUrl(url)).append(",");
        info.append("path=").append(path).append(",");
        info.append("veriftInfo=").append(verifyInfo).append(",");
        info.append("endReason=").append(std::to_string(CAST_INT(endReason))).append(",");
        info.append("status=").append(std::to_string(CAST_INT(status))).append(",");
        info.append("dirType=").append(std::to_string(CAST_INT(dirType))).append(",");
        info.append("netType=").append(std::to_string(CAST_INT(netType))).append(",");
        info.append("method=").append(std::to_string(CAST_INT(method))).append(",");
        info.append("retryTimes=").append(std::to_string(retryTimes)).append(",");
        info.append("packageSize=").append(std::to_string(packageSize)).append(",");
        info.append("downloadedSize=").append(std::to_string(downloadedSize)).append(",");
        info.append("startTime=").append(std::to_string(startTime)).append(",");
        info.append("endTime=").append(std::to_string(endTime)).append(",");
        info.append("isNeedAutoResume=").append(std::to_string(isNeedAutoResume)).append(",");
        info.append("isIgnoredFail=").append(std::to_string(isIgnoredFail)).append(",");
        info.append("effectiveTime=").append(std::to_string(effectiveTime));
        return info;
    }

    void UpdateDownloadInfo(const DownloadCallbackInfo &info)
    {
        status = info.status;
        endReason = info.endReason;
        downloadedSize = info.downloadedSize;
        startTime = info.startTime;
        endTime = info.endTime;
        downloadErrorInfo = info.downloadErrorInfo;
        effectiveTime = info.effectiveTime;
        verifiedSize = info.verifiedSize;

        // 下载中，把重试次数清零，避免重试次数被上次失败继承
        if (info.resetRetryTimes) {
            retryTimes = 0;
        }
    }

    [[nodiscard]] bool IsNeedVerify() const
    {
        return (!IsCompleted() && packageSize == downloadedSize) || status == DownloadStatus::VERIFYING;
    }

    [[nodiscard]] bool IsCompleted() const
    {
        return status == DownloadStatus::SUCCESS || status == DownloadStatus::FAIL;
    }

    [[nodiscard]] bool IsManualPaused() const
    {
        return status == DownloadStatus::PAUSE;
    }

    [[nodiscard]] bool IsPaused() const
    {
        return status == DownloadStatus::PAUSE || status == DownloadStatus::AUTO_PAUSE;
    }

    [[nodiscard]] DownloadStatus GetTaskStatus() const
    {
        if (status == DownloadStatus::FAIL && isIgnoredFail) {
            return DownloadStatus::SUCCESS;
        }
        return status;
    }

    [[nodiscard]] bool IsRetryAble(const int32_t maxRetryTimes = 3) const
    {
        return retryTimes < maxRetryTimes;
    }
};

using OnDownloadCallback = std::function<void(DownloadCallbackInfo callbackInfo)>;
} // namespace OHOS::UpdateService
#endif // DOWNLOAD_COMMON_H
