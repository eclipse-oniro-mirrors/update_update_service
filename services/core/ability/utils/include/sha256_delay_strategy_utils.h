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

#ifndef DUPDATE_SHA256_DELAY_STRATEGY_UTILS_H
#define DUPDATE_SHA256_DELAY_STRATEGY_UTILS_H

#include <functional>
#include <map>

#include "singleton.h"

using Sha256DelayStrategyCallbackFunc = std::function<bool()>;

namespace OHOS::UpdateService {
class Sha256DelayStrategyManager : public DelayedSingleton<Sha256DelayStrategyManager> {
    DECLARE_DELAYED_SINGLETON(Sha256DelayStrategyManager);

public:
    void StoreDelayStrategy(const std::string &key, const Sha256DelayStrategyCallbackFunc &delayCallback);
    bool IsScreenOnAndOUCNotDisplay(const std::string &key);

private:
    std::map<std::string, Sha256DelayStrategyCallbackFunc> delayStrategyMap_;
};
} // namespace OHOS::UpdateService
#endif // DUPDATE_SHA256_DELAY_STRATEGY_UTILS_H