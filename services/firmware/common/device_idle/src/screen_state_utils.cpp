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

#include "screen_state_utils.h"

#include "firmware_log.h"
#include "power_mgr_client.h"

namespace OHOS {
namespace UpdateService {
bool ScreenStateUtils::IsScreenOn()
{
    auto& powerClient = OHOS::PowerMgr::PowerMgrClient::GetInstance();
    bool isScreenOn = powerClient.IsScreenOn();
    FIRMWARE_LOGI("ScreenStateUtils::IsScreenOn %{public}s", isScreenOn ? "true" : "false");
    return isScreenOn;
}
} // namespace UpdateService
} // namespace OHOS
