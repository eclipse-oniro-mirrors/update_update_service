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

#ifndef DOWNLOAD_REQUEST_H
#define DOWNLOAD_REQUEST_H

#include <curl/curl.h>
#include <functional>
#include <memory>
#include <vector>

#include "network_base_request.h"

namespace OHOS::UpdateService {
using DownloadProgressCallback = std::function<void(int64_t dlTotal, int64_t dlNow)>;

struct DownloadRequestCallback {
    DownloadProgressCallback onProgress = nullptr;
};

enum class FunctionResult {
    SUCCESS = 0,
    FAIL = 1,
};

class DownloadRequest : public NetworkBaseRequest {
public:
    DownloadRequest();
    ~DownloadRequest() override;

    DownloadRequest &SetUrl(const std::string &url);
    DownloadRequest &SetMethod(RequestMethod method);
    DownloadRequest &SetHeader(const std::map<std::string, std::string> &header);
    DownloadRequest &SetRequestBody(const std::string &params);
    DownloadRequest &SetFile(std::shared_ptr<FILE> filePtr);
    DownloadRequest &SetDownloadedSize(int64_t downloadedSize);
    DownloadRequest &SetCallback(DownloadRequestCallback callback);
    void SetDownloadStopStatus(bool isNeedStop);
    bool GetDownloadStopStatus();
    int64_t GetDownloadedSize();
    std::shared_ptr<FILE> GetFile();
    std::vector<std::string> GetHeadersBuffer();
    DownloadRequestCallback GetCallback();
    void AddFsyncSize(size_t writeSize);
    size_t GetFsyncSize();
    void ClearSyncSize();

private:
    void SetExtraHeader(struct curl_slist **headers) final;
    void PerformExtraCurlRequest(CURL *curl, NetworkResponse &response) final;

    void SetPostResumeOption(struct curl_slist **headers);
    void SetGetResumeOption(CURL *curl);

    static FunctionResult CallbackProgress(void *clientp, curl_off_t dlTotal, curl_off_t dlNow, curl_off_t ulTotal,
        curl_off_t ulNow);
    static size_t HeaderCallback(void *contents, size_t size, size_t nmemb, void *userp);
    static size_t WriteFun(const void *buffer, size_t size, size_t nmemb, void *saveInfo);

private:
    std::shared_ptr<FILE> file_ = nullptr;
    int64_t downloadedSize_ = 0;
    bool isNeedStop_ = false;
    std::vector<std::string> headersBuffer_;
    DownloadRequestCallback callback_;
    size_t fsyncSize_ = 0;
};
} // namespace OHOS::UpdateService
#endif // DOWNLOAD_REQUEST_H