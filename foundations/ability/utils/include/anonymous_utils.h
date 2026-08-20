/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANONYMOUS_UTILS_H
#define ANONYMOUS_UTILS_H

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <stdlib.h>
#include <string>

namespace OHOS {
namespace UpdateService {
static const int32_t ENCRYPT_LENGTH = 4; // 需要替换*的长度
static const int32_t ENCRYPT_TOTAL_LENGTH = 8; // 敏感数据匿名化后最长长度
static const std::string ENCRYPT_STR = "****";

class AnonymousUtils {
public:
    static std::string AnonymousUrl(const std::string &url)
    {
        std::string encryptUrl = url;
        std::string httpsPrefix = "https://";
        std::string httpPrefix = "http://";

        // 从https:// 或者 http:// 开始后面4位替换为 xxxx
        if (encryptUrl.compare(0, httpsPrefix.size(), httpsPrefix) == 0) {
            encryptUrl.replace(httpsPrefix.size(), ENCRYPT_LENGTH, ENCRYPT_STR);
            return encryptUrl;
        }
        if (encryptUrl.compare(0, httpPrefix.size(), httpPrefix) == 0) {
            encryptUrl.replace(httpPrefix.size(), ENCRYPT_LENGTH, ENCRYPT_STR);
            return encryptUrl;
        }
        return encryptUrl;
    }

    static std::string AnonymousString(std::string inputStr)
    {
        if (inputStr.empty()) {
            return inputStr;
        }
        std::string result;
        size_t length = inputStr.length();
        if (length >= ENCRYPT_TOTAL_LENGTH) {
            std::string sequence = inputStr.substr(0, ENCRYPT_LENGTH);
            result = sequence + ENCRYPT_STR;
        } else if (length > ENCRYPT_LENGTH) {
            std::string sequence = inputStr.substr(0, length - ENCRYPT_LENGTH);
            result = sequence + ENCRYPT_STR;
        } else {
            result = ENCRYPT_STR;
        }
        return result;
    }

    static std::string GetPrintFilePathStr(const std::string &str)
    {
        std::filesystem::path p(str);
        std::string lastPart = p.filename().string();
        std::string parentPath = p.parent_path().string();
        std::string printDir = ReplacePercentageWithAsterisks(lastPart);
        return parentPath + "/" + printDir;
    }

private:
    static std::string ReplacePercentageWithAsterisks(const std::string &str)
    {
        const double percentage = 30.0;
        const double oneHundred = 100.0;
        int32_t replaceCount = static_cast<int32_t>(str.length() * percentage / oneHundred);
        std::string result = str;
        int32_t index = static_cast<int32_t>(result.length()) - replaceCount;
        if (index < 0) {
            return result;
        }
        for (int32_t i = 0; i < static_cast<int32_t>(result.length()); ++i) {
            if (i >= index) {
                result[i] = '*';
            }
        }
        return result;
    }
};
} // namespace UpdateService
} // namespace OHOS
#endif // ANONYMOUS_UTILS_H