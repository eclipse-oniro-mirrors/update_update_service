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

#include "sha256_utils.h"

#include <cstdlib>
#include <thread>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>

#include "file_utils.h"

namespace OHOS {
namespace UpdateService {
constexpr int OPENSSL_SUCCESS = 1;
constexpr unsigned int SHA256_STRING_LEN = 65;
constexpr unsigned int SHA256_LENGTH = 32;
constexpr unsigned int MAX_BUFFER_LENGTH = 1024;
constexpr unsigned int SHA256_TO_STRING_STEP = 2;

std::string Sha256Utils::CalculateHashCode(std::string inputStr)
{
    char result[SHA256_STRING_LEN] = {0};
    if (!Sha256Calculate(reinterpret_cast<const unsigned char *>(inputStr.c_str()), inputStr.length(),
        result, SHA256_STRING_LEN)) {
        ENGINE_LOGE("CalculateHashCode fail, src = %s", inputStr.c_str());
        return "";
    }
    return result;
}

bool Sha256Utils::CheckFileSha256String(const std::string &fileName, const std::string &sha256String)
{
    if (!FileUtils::IsFileExist(fileName)) {
        ENGINE_LOGE("check file sha256 failed, file not exist");
        return false;
    }
    char sha256Result[SHA256_STRING_LEN] = {0}; // sha256Result len is 65
    if (!GetFileSha256Str(fileName, sha256Result, sizeof(sha256Result))) {
        ENGINE_LOGE("get file sha256 failed");
        return false;
    }
    if (strcasecmp(sha256Result, sha256String.c_str()) != 0) {
        ENGINE_LOGE("sha256 not same! input=%{public}s, cal=%{public}s", sha256Result, sha256String.c_str());
        return false;
    }
    return true;
}

Sha256VerifyResult Sha256Utils::GetFileSha256VerifyResult(const std::string& fileName, const std::string& sha256String,
    const OnFileVerifyCallback& callback)
{
    if (!FileUtils::IsFileExist(fileName)) {
        ENGINE_LOGE("check file sha256 failed, fileName = %{public}s is not exist", fileName.c_str());
        return {
            .sha256ErrCode = Sha256ErrCode::FILE_NOT_EXIST
        };
    }
    char sha256Result[SHA256_STRING_LEN] = {0}; // sha256Result len is 65
    if (!GetFileSha256Str(fileName, sha256Result, sizeof(sha256Result), callback)) {
        ENGINE_LOGE("get file sha256 failed");
        return {
            .sha256ErrCode = Sha256ErrCode::OBTAIN_FILE_SHA256_FAIL,
            .sha256Result = sha256Result
        };
    }
    if (strcasecmp(sha256Result, sha256String.c_str()) != 0) {
        ENGINE_LOGE("sha256 not same! expected=%{public}s, cal=%{public}s", sha256String.c_str(), sha256Result);
        return {
            .sha256ErrCode = Sha256ErrCode::SHA256_NOT_MATCH,
            .sha256Result = sha256Result
        };
    }
    return {
        .sha256ErrCode = Sha256ErrCode::SUCCESS
    };
}

bool Sha256Utils::GetDigestFromFile(const char* fileName, unsigned char digest[], const OnFileVerifyCallback& callback)
{
    char realPath[PATH_MAX] = {};
    if (realpath(fileName, realPath) == NULL) {
        return false;
    }
    std::ifstream file(realPath, std::ios::binary);
    if (!file.is_open()) {
        ENGINE_LOGI("%{private}s Unable to open file", realPath);
        return false;
    }
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    size_t MAX_BUFFER_LENGTH = 1024;
    char *buffer = (char *)malloc(MAX_BUFFER_LENGTH);
    if (buffer == nullptr) {
        ENGINE_LOGI("failed to allocate memory");
        file.close();
        return false;
    }

    Sha256DelayParam delayParam;
    auto delayStrategy = Sha256DelayStrategyMap_.find(sha256DelayScene_);
    if (delayStrategy != Sha256DelayStrategyMap_.end()) {
        delayParam = delayStrategy->second;
    }
    int64_t cycleCount = 0;
    std::streamsize bytesRead = 0;
    while (!file.eof()) {
        file.read(buffer, MAX_BUFFER_LENGTH);
        SHA256_Update(&ctx, buffer, file.gcount());
        if (isNeedStop_) {
            ENGINE_LOGI("get digest from file stop");
            break;
        }
        cycleCount++;
        if (delayParam.isNeedDelay && (cycleCount % delayParam.delayCycleCount == 0)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayParam.sleepDurationPerTimes));
        }
        bytesRead += file.gcount();
        if (callback != nullptr) {
            callback((int64_t)bytesRead);
        }
    }
    SHA256_Final(digest, &ctx);
    file.close();
    free(buffer);
    return true;
}

void Sha256Utils::SetDelayStatus(Sha256DelayScene scene)
{
    ENGINE_LOGI("sha256DelayScene is %{public}d", static_cast<int32_t>(scene));
    sha256DelayScene_ = scene;
}

void Sha256Utils::FreeBuffer(char *buffer, std::ifstream &file)
{
    if (buffer != nullptr) {
        free(buffer);
    }

    if (file.is_open()) {
        file.close();
    }
}

bool Sha256Utils::GetFileSha256Str(const std::string& fileName, char* sha256Result, uint32_t len,
    const OnFileVerifyCallback& callback)
{
    unsigned char digest[SHA256_LENGTH] = {0};
    GetDigestFromFile(fileName.c_str(), digest, callback);
    return TransDigestToSha256Result(sha256Result, len, digest);
}

bool Sha256Utils::GetDigestFromFile(const char *fileName, unsigned char digest[])
{
    char realPath[PATH_MAX] = {};
    if (realpath(fileName, realPath) == NULL) {
        ENGINE_LOGI("file not exist or invalid");
        return false;
    }

    std::ifstream file(realPath, std::ios::binary);
    if (!file.is_open()) {
        ENGINE_LOGI("%{private}s Unable to open file", realPath);
        return false;
    }

    char *buffer = (char *)malloc(MAX_BUFFER_LENGTH); /* buffer len 1024 */
    if (buffer == nullptr) {
        ENGINE_LOGI("failed to allocate memory");
        file.close();
        return false;
    }
    SHA256_CTX sha256;
    int32_t startRet = SHA256_Init(&sha256);
    if (startRet != OPENSSL_SUCCESS) {
        ENGINE_LOGE("SHA256_init_ret failed, startRet = %{public}d", startRet);
        FreeBuffer(buffer, file);
        return false;
    }

    while (!file.eof()) {
        file.read(buffer, MAX_BUFFER_LENGTH);
        int32_t updateRet = SHA256_Update(&sha256, buffer, file.gcount());
        if (updateRet != OPENSSL_SUCCESS) {
            ENGINE_LOGE("SHA256_update_ret failed, updateRet = %{public}d", updateRet);
            FreeBuffer(buffer, file);
            return false;
        }
    }
    int32_t finishRet = SHA256_Final(digest, &sha256);
    if (finishRet != OPENSSL_SUCCESS) {
        ENGINE_LOGE("SHA256_finish_ret failed, finishRet = %{public}d", finishRet);
        FreeBuffer(buffer, file);
        return false;
    }
    FreeBuffer(buffer, file);
    return true;
}

bool Sha256Utils::GetFileSha256Str(const std::string &fileName, char *sha256Result, uint32_t len)
{
    unsigned char digest[SHA256_LENGTH] = {0};
    GetDigestFromFile(fileName.c_str(), digest);
    return TransDigestToSha256Result(sha256Result, len, digest);
}

bool Sha256Utils::Sha256Calculate(const unsigned char *input, size_t len, char *componentId,
    unsigned int componentIdLen)
{
    unsigned char digest[SHA256_LENGTH] = {0};
    SHA256_CTX ctx;
    int ret = memset_s(&ctx, sizeof(ctx), 0, sizeof(ctx));
    if (ret != 0) {
        ENGINE_LOGE("init sha256_context failed");
        return false;
    }
    int startRet = SHA256_Init(&ctx);
    if (startRet != OPENSSL_SUCCESS) {
        ENGINE_LOGE("SHA256_Init failed, startRet = %{public}d", startRet);
        return false;
    }

    int updateRet = SHA256_Update(&ctx, input, len);
    if (updateRet != OPENSSL_SUCCESS) {
        ENGINE_LOGE("SHA256_Update failed, updateRet = %{public}d", updateRet);
        return false;
    }

    int finishRet = SHA256_Final(digest, &ctx);
    if (finishRet != OPENSSL_SUCCESS) {
        ENGINE_LOGE("SHA256_Final failed, finishRet = %{public}d", finishRet);
        return false;
    }

    return TransDigestToSha256Result(componentId, componentIdLen, digest);
}

bool Sha256Utils::TransDigestToSha256Result(char *sha256Result, uint32_t componentIdLen, const unsigned char *digest)
{
    for (unsigned int i = 0; i < SHA256_LENGTH; i++) {
        unsigned int deviation = i * SHA256_TO_STRING_STEP;
        if (deviation >= componentIdLen) {
            ENGINE_LOGE("deviation len illegal");
            return false;
        }
        int result = sprintf_s(sha256Result + deviation, (componentIdLen - deviation), "%02x", digest[i]);
        if (result <= 0) {
            ENGINE_LOGE("generated sha256 failed");
            return false;
        }
    }
    return true;
}
} // namespace UpdateService
} // namespace OHOS