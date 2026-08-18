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

#include "download_request.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "anonymous_utils.h"
#include "constant.h"
#include "string_utils.h"
#include "update_log.h"

namespace OHOS::UpdateService {
namespace {
constexpr long CONNECT_TIMEOUT = 20L;
constexpr int32_t FILE_SIZE_MB = Constant::MIN_FILE_DOWNLOAD_SIZE - CURL_MAX_WRITE_SIZE * 2;
}

DownloadRequest::DownloadRequest()
{
    ENGINE_LOGI("DownloadRequest");
}

DownloadRequest::~DownloadRequest()
{
    ENGINE_LOGI("~DownloadRequest");
}

void DownloadRequest::SetExtraHeader(struct curl_slist **headers)
{
    SetPostResumeOption(headers);
}

void DownloadRequest::PerformExtraCurlRequest(CURL *curl, NetworkResponse &response)
{
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DownloadRequest::WriteFun);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, DownloadRequest::CallbackProgress);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, DownloadRequest::HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headersBuffer_);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT); // 连接服务器超时时间

    SetGetResumeOption(curl);
}

void DownloadRequest::SetPostResumeOption(struct curl_slist **headers)
{
    if (downloadedSize_ <= 0) {
        return;
    }
    std::string resumeOption;
    resumeOption.append("Range: bytes=").append(std::to_string(downloadedSize_)).append("-");
    ENGINE_LOGI("post resume download option = [%{public}s]", resumeOption.c_str());
    curl_slist_append(*headers, resumeOption.c_str());
}

void DownloadRequest::SetGetResumeOption(CURL *curl)
{
    if (downloadedSize_ > 0 && method_ == RequestMethod::GET) {
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, downloadedSize_);
    }
}

size_t DownloadRequest::HeaderCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    if (contents == nullptr || size == 0 || nmemb == 0 || userp == nullptr) {
        ENGINE_LOGE("contents is nullptr or size is 0 or nmemb is 0 or userp is nullptr");
        return 0;
    }

    constexpr int64_t maxSize = 16 * 1024; // 16KB 限制大小为16KB http头部信息最大长度是8kb
    if (size * nmemb > maxSize) {
        ENGINE_LOGE("header size is too large");
        return 0;
    }

    std::string headers;
    headers.assign(static_cast<char *>(contents), size * nmemb);
    StringUtils::Trim(headers);
    static_cast<std::vector<std::string> *>(userp)->push_back(headers);
    return size * nmemb;
}

std::vector<std::string> DownloadRequest::GetHeadersBuffer()
{
    return headersBuffer_;
}

/* 用于返回下载百分比回调 */
FunctionResult DownloadRequest::CallbackProgress(void *clientp, curl_off_t dlTotal, curl_off_t dlNow,
    curl_off_t ulTotal, curl_off_t ulNow)
{
    if (dlTotal == 0 && dlNow == 0) {
        return FunctionResult::SUCCESS;
    }

    DownloadRequest *request = (DownloadRequest *)clientp;
    if (request == nullptr) {
        ENGINE_LOGE("DownloadRequest is nullptr");
        return FunctionResult::FAIL;
    }
    if (request->GetDownloadStopStatus()) {
        ENGINE_LOGI("DownloadRequest stopped by user operation");
        return FunctionResult::FAIL;
    }

    int64_t downloadedSize = request->GetDownloadedSize();
    auto callback = request->GetCallback().onProgress;
    if (callback == nullptr) {
        ENGINE_LOGE("DownloadRequest callback is nullptr");
        return FunctionResult::FAIL;
    }
    callback(dlTotal + downloadedSize, dlNow + downloadedSize);
    return FunctionResult::SUCCESS;
}

size_t DownloadRequest::WriteFun(const void *buffer, size_t size, size_t nmemb, void *saveInfo)
{
    if (size == 0 || nmemb == 0 || CURL_MAX_WRITE_SIZE < (long long)size * (long long)nmemb) {
        ENGINE_LOGI("write data out of available size: %{public}zu,%{public}zu", size, nmemb);
        return 0;
    }
    size_t rSize = size * nmemb;

    DownloadRequest *request = (DownloadRequest *)saveInfo;
    if (request == nullptr) {
        ENGINE_LOGE("DownloadRequest is nullptr");
        return 0;
    }

    if (request->GetDownloadStopStatus()) {
        ENGINE_LOGI("DownloadRequest stopped by user operation");
        return 0;
    }

    std::shared_ptr<FILE> fp = request->GetFile();
    if (fp == nullptr) {
        ENGINE_LOGE("open file failed");
        return 0;
    }

    if (fwrite(buffer, size, nmemb, fp.get()) != nmemb) {
        ENGINE_LOGE("write buff err");
        return 0;
    }
    request->AddFsyncSize(rSize);
    if (request->GetFsyncSize() >= FILE_SIZE_MB) {
        FILE *file = fp.get();
        int32_t ret = fflush(file);
        if (ret != 0) {
            ENGINE_LOGE("flush file fail, errno %{public}d", errno);
            return 0;
        }
        ret = fsync(fileno(file));
        if (ret != 0) {
            ENGINE_LOGE("fsync file fail, errno %{public}d", errno);
            return 0;
        }
        request->ClearSyncSize();
    }
    return rSize;
}

DownloadRequest &DownloadRequest::SetUrl(const std::string &url)
{
    url_ = url;
    return *this;
}

DownloadRequest &DownloadRequest::SetHeader(const std::map<std::string, std::string> &header)
{
    headerParameters_ = header;
    return *this;
}

DownloadRequest &DownloadRequest::SetMethod(RequestMethod method)
{
    method_ = method;
    return *this;
}

DownloadRequest &DownloadRequest::SetRequestBody(const std::string &params)
{
    requestBody_ = params;
    return *this;
}

DownloadRequest &DownloadRequest::SetFile(std::shared_ptr<FILE> file)
{
    file_ = file;
    return *this;
}

DownloadRequest &DownloadRequest::SetDownloadedSize(int64_t downloadedSize)
{
    downloadedSize_ = downloadedSize;
    return *this;
}

DownloadRequest &DownloadRequest::SetCallback(DownloadRequestCallback callback)
{
    callback_ = callback;
    return *this;
}

void DownloadRequest::SetDownloadStopStatus(bool isNeedStop)
{
    isNeedStop_ = isNeedStop;
}

bool DownloadRequest::GetDownloadStopStatus()
{
    return isNeedStop_;
}

int64_t DownloadRequest::GetDownloadedSize()
{
    return downloadedSize_;
}

std::shared_ptr<FILE> DownloadRequest::GetFile()
{
    return file_;
}

DownloadRequestCallback DownloadRequest::GetCallback()
{
    return callback_;
}

void DownloadRequest::AddFsyncSize(size_t writeSize)
{
    fsyncSize_ += writeSize;
}

size_t DownloadRequest::GetFsyncSize()
{
    return fsyncSize_;
}

void DownloadRequest::ClearSyncSize()
{
    fsyncSize_ = 0;
}
} // namespace OHOS::UpdateService
