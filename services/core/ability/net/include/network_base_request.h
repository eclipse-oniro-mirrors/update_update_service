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

#ifndef NETWORK_BASE_REQUEST_H
#define NETWORK_BASE_REQUEST_H

#include <chrono>
#include <curl/curl.h>
#include <map>
#include <string>

#include "network_response.h"

namespace OHOS::UpdateService {
constexpr long DEFAULT_REQUEST_TIMEOUT = 20L;
class NetworkBaseRequest {
public:
    NetworkBaseRequest() = default;
    virtual ~NetworkBaseRequest() = default;

    NetworkResponse DoNetRequest();

    NetworkBaseRequest &SetRetry(int32_t maxRetryTimes, int32_t retryInterval);
    NetworkBaseRequest &SetTimeOut(long timeOut);

protected:
    std::string url_;
    RequestMethod method_;
    std::map<std::string, std::string> headerParameters_;
    std::string requestBody_;
    int32_t maxRetryTimes_ = 0;
    int32_t retryInterval_ = 0;
    long requestTimeout_ = DEFAULT_REQUEST_TIMEOUT;

private:
    bool IsUrlValid();
    CURL *InitCurl();
    bool IsHttpsUrl();

    struct curl_slist *SetCurlHeaders(CURL *curl);
    virtual void SetExtraHeader(struct curl_slist **headers){};
    void SetCurlPostBody(CURL *curl);
    void PerformCurlRequest(CURL *curl, NetworkResponse &response);
    virtual void PerformExtraCurlRequest(CURL *curl, NetworkResponse &response){};
    void CleanupCurlParams(CURL *curl, struct curl_slist *headers);
};
} // namespace OHOS::UpdateService
#endif // NETWORK_BASE_REQUEST_H