/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "call_state_utils.h"

#include "call_manager_base.h"
#include "call_manager_client.h"
#include "firmware_log.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace UpdateService {
bool CallStateUtils::HasCall()
{
    std::shared_ptr<OHOS::Telephony::CallManagerClient> callClient =
        OHOS::DelayedSingleton<OHOS::Telephony::CallManagerClient>::GetInstance();
    if (callClient == nullptr) {
        FIRMWARE_LOGI("CallStateUtils::HasCall callClient is null");
        return false;
    }
    callClient->Init(TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID);
    bool hasCall = callClient->HasCall(true);
    FIRMWARE_LOGI("CallStateUtils::HasCall %{public}s", hasCall ? "true" : "false");
    return hasCall;
}
} // namespace UpdateService
} // namespace OHOS
