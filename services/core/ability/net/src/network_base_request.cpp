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

#include "network_base_request.h"

#include <openssl/ssl.h>
#include <thread>

#include "dupdate_errno.h"
#include "string_utils.h"
#include "update_log.h"

namespace OHOS::UpdateService {
namespace {
constexpr long LOW_SPEED_LIMIT = 1L;
constexpr long LOW_SPEED_TIME = 6L;
constexpr long MAXREDIRS_SIZE = 5L;
constexpr std::string_view HTTPS_PREFIX = "https://";
constexpr std::string_view HTTP_PREFIX = "http://";
}

NetworkResponse NetworkBaseRequest::DoNetRequest()
{
    NetworkResponse response;
    if (!IsUrlValid()) {
        ENGINE_LOGE("DoNetRequest curl is invalid");
        return response;
    }

    CURL *curl = InitCurl();
    if (curl == nullptr) {
        ENGINE_LOGE("DoNetRequest curl is nullptr");
        return response;
    }
    struct curl_slist *headers = SetCurlHeaders(curl);
    SetCurlPostBody(curl);
    PerformCurlRequest(curl, response);
    CleanupCurlParams(curl, headers);
    return response;
}

bool NetworkBaseRequest::IsUrlValid()
{
    if ((url_.rfind(HTTP_PREFIX.data(), 0) != 0) && (url_.rfind(HTTPS_PREFIX.data(), 0) != 0)) {
        ENGINE_LOGE("NetworkBaseRequest IsUrlValid url_ is error");
        return false;
    }
    return true;
}

CURL *NetworkBaseRequest::InitCurl()
{
    CURL *curl = curl_easy_init();
    if (curl == nullptr) {
        return nullptr;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIME); // 设置超时时间，6秒下载不到1字节则超时
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // 在多线程场景下，若不设置CURLOPT_NOSIGNAL选项，可能会crash
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);        // 跟随跳转
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, MAXREDIRS_SIZE); // 最大五跳
    return curl;
}

bool NetworkBaseRequest::IsHttpsUrl()
{
    return url_.rfind(HTTPS_PREFIX.data(), 0) == 0;
}

struct curl_slist *NetworkBaseRequest::SetCurlHeaders(CURL *curl)
{
    if (method_ != RequestMethod::POST && headerParameters_.empty()) {
        return nullptr;
    }
    curl_slist *headers = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");
    for (auto &iter : headerParameters_) {
        curl_slist_append(headers, std::string(iter.first).append(":").append(iter.second).c_str());
    }

    SetExtraHeader(&headers);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    return headers;
}

void NetworkBaseRequest::SetCurlPostBody(CURL *curl)
{
    if (method_ != RequestMethod::POST) {
        return;
    }
    curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, requestBody_.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1);
}

void NetworkBaseRequest::PerformCurlRequest(CURL *curl, NetworkResponse &response)
{
    PerformExtraCurlRequest(curl, response);
    CURLcode retCurlCode = CURLcode::CURLE_OK;
    int32_t retryTimes = 0;
    response.retryInfo.append(url_).append(":");
    do {
        retCurlCode = curl_easy_perform(curl);
        response.result = static_cast<int32_t>(retCurlCode);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(response.status));
        ENGINE_LOGI("curl_easy_perform() finish: ret %{public}d, error cause %{public}s, httpCode %{public}" PRId64 "",
            response.result, curl_easy_strerror(retCurlCode), response.status);

        response.retryInfo.append(std::to_string(response.result)).append(",");

        if (retCurlCode == CURLcode::CURLE_OK) {
            break;
        }

        if (retryTimes++ < maxRetryTimes_ && retryInterval_ > 0) {
            ENGINE_LOGI("PerformCurlRequest fail, will retry after %{public}" PRId32 " milliseconds", retryInterval_);
            std::this_thread::sleep_for(std::chrono::milliseconds(retryInterval_));
        } else {
            ENGINE_LOGE("PerformCurlRequest fail after try %{public}" PRId32 " times", retryTimes);
            break;
        }
    } while (retryTimes <= maxRetryTimes_);
    response.retryInfo.pop_back();
    response.retryInfo.append("|").append(std::to_string(retryTimes));
    if (retryTimes == 0) {
        response.retryInfo.clear();
    }
}

void NetworkBaseRequest::CleanupCurlParams(CURL *curl, struct curl_slist *headers)
{
    curl_easy_cleanup(curl);
    if (headers != nullptr) {
        curl_slist_free_all(headers);
    }
}

NetworkBaseRequest &NetworkBaseRequest::SetRetry(int32_t maxRetryTimes, int32_t retryInterval)
{
    maxRetryTimes_ = maxRetryTimes;
    retryInterval_ = retryInterval;
    return *this;
}

NetworkBaseRequest &NetworkBaseRequest::SetTimeOut(long timeOut)
{
    if (timeOut > 0) {
        requestTimeout_ = timeOut;
    } else {
        requestTimeout_ = DEFAULT_REQUEST_TIMEOUT;
    }
    return *this;
}
} // namespace OHOS::UpdateService
