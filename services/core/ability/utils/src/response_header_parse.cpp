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

#include "response_header_parse_utils.h"

#include "string_utils.h"

namespace OHOS::UpdateService {
std::string ResponseHeaderParseUtils::FormatHeader(const std::vector<std::string> &headers)
{
    if (headers.empty()) {
        return "";
    }

    std::string newHeader;
    for (const auto &header : headers) {
        newHeader.append(header);
    }
    return newHeader;
}

std::map<std::string, std::string> ResponseHeaderParseUtils::ParseKeysFromHeader(
    const std::vector<std::string> &headers, const std::set<std::string> &keys)
{
    std::map<std::string, std::string> headerImportantInfoMap;
    if (headers.empty()) {
        return headerImportantInfoMap;
    }

    for (const auto &header : headers) {
        size_t colonPos = header.find(':');
        if (colonPos != std::string::npos && colonPos != header.length() - 1) {
            std::string key = StringUtils::SafeSubString(header, 0, colonPos, "");
            if (key.empty()) {
                continue;
            }

            StringUtils::Trim(key);
            auto it = keys.find(key);
            if (it != keys.end()) {
                std::string value =
                    StringUtils::SafeSubString(header, colonPos + 1, header.length() - colonPos - 1, "");
                StringUtils::Trim(value);
                headerImportantInfoMap[key] = value;
            }
        }
    }
    return headerImportantInfoMap;
}

std::string ResponseHeaderParseUtils::ParseKeyFromHeader(const std::vector<std::string> &headers,
    const std::string &importKey)
{
    if (headers.empty()) {
        return "";
    }

    for (const auto &header : headers) {
        size_t colonPos = header.find(':');
        if (colonPos != std::string::npos && colonPos != header.length() - 1) {
            std::string key = StringUtils::SafeSubString(header, 0, colonPos, "");
            if (key.empty()) {
                continue;
            }

            StringUtils::Trim(key);
            if (key == importKey) {
                std::string value =
                    StringUtils::SafeSubString(header, colonPos + 1, header.length() - colonPos - 1, "");
                StringUtils::Trim(value);
                return value;
            }
        }
    }
    return "";
}

std::string ResponseHeaderParseUtils::ParseFirstMsgFromHeader(const std::vector<std::string> &headers)
{
    if (headers.empty()) {
        return "";
    }

    return headers.front();
}

bool ResponseHeaderParseUtils::FindKeyFromHeader(const std::vector<std::string> &headers, const std::string &importKey)
{
    auto iter = std::find_if(headers.begin(), headers.end(), [importKey](const std::string &header) {
        size_t colonPos = header.find(':');
        std::string key = StringUtils::SafeSubString(header, 0, colonPos, "");
        if (key.empty()) {
            return false;
        }
        StringUtils::Trim(key);
        return key == importKey;
    });
    if (iter == headers.end()) {
        return false;
    }
    return true;
}
} // namespace OHOS::UpdateService