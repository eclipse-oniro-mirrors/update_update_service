/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "user_account_helper.h"

#include <chrono>
#include <thread>

#include "errors.h"
#include "os_account_manager.h"
#include "update_log.h"

namespace OHOS {
namespace UpdateService {
int32_t UserAccountHelper::GetActiveUserId() const
{
    constexpr int32_t retryInterval = 1000;
    constexpr int32_t maxRetryTimes = 10;
    int32_t currentUserId = -1;
    int32_t retryCount = 0;
    do {
        ErrCode result = AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(currentUserId);
        if (result == ERR_OK) {
            return currentUserId;
        }
        retryCount++;
        ENGINE_LOGI("GetActiveUserId, userId error, retry interval %{public}d milliseconds, retry "
            "%{public}d times", retryInterval, retryCount);
        std::this_thread::sleep_for(std::chrono::milliseconds(retryInterval));
    } while (retryCount < maxRetryTimes);
    return currentUserId;
}

bool UserAccountHelper::IsActiveUserUnlocked(int32_t userId) const
{
    if (userId < 0) {
        ENGINE_LOGE("Active os account id is invalid");
        return false;
    }

    bool isVerified = false;
    ErrCode ret = AccountSA::OsAccountManager::IsOsAccountVerified(userId, isVerified);
    if (ret != ERR_OK) {
        ENGINE_LOGE("Failed to query active user unlock state, userId %{public}d, ret %{public}d",
            userId, ret);
        return false;
    }

    ENGINE_LOGI("Active userId %{public}d unlocked %{public}s",
        userId, isVerified ? "true" : "false");
    return isVerified;
}
} // namespace UpdateService
} // namespace OHOS
