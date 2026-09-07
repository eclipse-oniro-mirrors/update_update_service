/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef SHA256_UTILS_H
#define SHA256_UTILS_H

#include <functional>
#include <map>
#include <string>

#include "securec.h"

#include "update_log.h"

namespace OHOS {
namespace UpdateService {
using OnFileVerifyCallback = std::function<void(int64_t verifiedSize)>;

enum class Sha256ErrCode {
    SUCCESS = 0,
    FILE_NOT_EXIST = -1,
    SHA256_NOT_MATCH = -2,
    OBTAIN_FILE_SHA256_FAIL = -3
};

enum class Sha256DelayScene {
    SCREEN_ON_AND_OUC_NOT_DISPLAY = 0,
    SCREEN_OFF_OR_OUC_DISPLAY = 1
};

struct Sha256VerifyResult {
    Sha256ErrCode sha256ErrCode = Sha256ErrCode::SUCCESS;
    std::string sha256Result;
};

struct Sha256DelayParam {
    bool isNeedDelay = true;
    int64_t delayCycleCount = 1300;
    int64_t sleepDurationPerTimes = 1;
};

class Sha256Utils {
public:
    static std::string CalculateHashCode(std::string inputStr);
    static bool CheckFileSha256String(const std::string &fileName, const std::string &sha256String);
    Sha256VerifyResult GetFileSha256VerifyResult(const std::string &fileName, const std::string &sha256String,
        const OnFileVerifyCallback &callback = nullptr);
    bool GetDigestFromFile(const char *fileName, unsigned char digest[], const OnFileVerifyCallback &callback);
    void SetDelayStatus(Sha256DelayScene scene);

private:
    static bool GetFileSha256Str(const std::string &fileName, char *sha256Result, uint32_t len);
    static bool GetDigestFromFile(const char *fileName, unsigned char digest[]);
    static bool Sha256Calculate(const unsigned char *input, size_t len, char *componentId, unsigned int componentIdLen);
    static bool TransDigestToSha256Result(char *sha256Result, uint32_t componentIdLen, const unsigned char *digest);
    static void FreeBuffer(char *buffer, std::ifstream &file);
    bool GetFileSha256Str(const std::string &fileName, char *sha256Result, uint32_t len,
        const OnFileVerifyCallback &callback);

    Sha256DelayScene sha256DelayScene_ = Sha256DelayScene::SCREEN_ON_AND_OUC_NOT_DISPLAY;
    std::map<Sha256DelayScene, Sha256DelayParam> Sha256DelayStrategyMap_ = {
        { Sha256DelayScene::SCREEN_ON_AND_OUC_NOT_DISPLAY, { true, 1300, 1 } },
        { Sha256DelayScene::SCREEN_OFF_OR_OUC_DISPLAY, { true, 4000, 1 } },
    };
    bool isNeedStop_ = false;
};
} // namespace UpdateService
} // namespace OHOS
#endif // SHA256_UTILS_H