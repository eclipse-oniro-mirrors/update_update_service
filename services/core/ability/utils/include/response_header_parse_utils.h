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

#ifndef UPDATE_RESPONSE_HEADER_PARSE_UTILS_H
#define UPDATE_RESPONSE_HEADER_PARSE_UTILS_H

#include <map>
#include <set>
#include <string>
#include <vector>

namespace OHOS::UpdateService {
class ResponseHeaderParseUtils {
public:
    static std::string FormatHeader(const std::vector<std::string> &headers);
    static std::map<std::string, std::string> ParseKeysFromHeader(const std::vector<std::string> &headers,
        const std::set<std::string> &keys);
    static std::string ParseKeyFromHeader(const std::vector<std::string> &headers, const std::string &importKey);
    static std::string ParseFirstMsgFromHeader(const std::vector<std::string> &headers);
    static bool FindKeyFromHeader(const std::vector<std::string> &headers, const std::string &importKey);
};
} // namespace OHOS::UpdateService
#endif // UPDATE_RESPONSE_HEADER_PARSE_UTILS_H
