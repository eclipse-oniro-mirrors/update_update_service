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

#include "sha256_delay_strategy_utils.h"
#include "string_utils.h"
#include "update_log.h"

namespace OHOS::UpdateService {
Sha256DelayStrategyManager::Sha256DelayStrategyManager()
{
    ENGINE_LOGD("Sha256DelayStrategyManager");
}

Sha256DelayStrategyManager::~Sha256DelayStrategyManager()
{
    ENGINE_LOGD("~Sha256DelayStrategyManager");
}

void Sha256DelayStrategyManager::StoreDelayStrategy(const std::string &key,
    const Sha256DelayStrategyCallbackFunc &delayCallback)
{
    if (key.empty() || delayCallback == nullptr) {
        return;
    }
    ENGINE_LOGI("StoreDelayStrategy, strategy key %{public}s", key.c_str());
    delayStrategyMap_[key] = delayCallback;
}

bool Sha256DelayStrategyManager::IsScreenOnAndOUCNotDisplay(const std::string &key)
{
    if (key.empty()) {
        return false;
    }
    auto delayStrategy = delayStrategyMap_.find(key);
    if (delayStrategy == delayStrategyMap_.end()) {
        return false;
    }
    bool isScreenOnAndOUCNotDisplay = delayStrategy->second();
    ENGINE_LOGI("IsScreenOnAndOUCNotDisplay %{public}s, strategy key %{public}s",
        StringUtils::GetBoolStr(isScreenOnAndOUCNotDisplay).c_str(), key.c_str());
    return isScreenOnAndOUCNotDisplay;
}
} // namespace OHOS::UpdateService