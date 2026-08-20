/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef NETWORK_RESPONSE_H
#define NETWORK_RESPONSE_H

#include <cstdint>
#include <string>

namespace OHOS::UpdateService {
namespace HttpRespondCode {
constexpr int32_t CODE_DEFAULT = -1;
constexpr int32_t CODE_SUCCESS = 200;
constexpr int32_t CODE_301 = 301;
constexpr int32_t CODE_302 = 302;
constexpr int32_t CODE_303 = 303;
constexpr int32_t CODE_307 = 307;
constexpr int32_t CODE_400 = 400;
constexpr int32_t CODE_503 = 503;
constexpr int32_t CODE_504 = 504;
}

enum class HttpConstant {
    DEFAULT = -1,
    SUCCESS = 200,
};

enum class RequestMethod {
    POST = 0,
    GET = 1,
};

class NetworkResponse {
public:
    bool IsContentValid() const
    {
        return !content.empty();
    }

    bool IsRequestSuccess() const
    {
        return status == static_cast<int64_t>(HttpRespondCode::CODE_SUCCESS);
    }

public:
    int64_t status = HttpRespondCode::CODE_DEFAULT;
    int32_t result = 0;
    std::string content;
    std::string retryInfo;
};
}
#endif // NETWORK_RESPONSE_H
