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

#include "device_idle_utils.h"

#include "call_state_utils.h"
#include "firmware_log.h"
#include "music_state_utils.h"
#include "screen_state_utils.h"

namespace OHOS {
namespace UpdateService {
bool DeviceIdleUtils::IsDeviceIdle()
{
    if (ScreenStateUtils::IsScreenOn()) {
        FIRMWARE_LOGI("IsDeviceIdle: screen is on, not idle");
        return false;
    }
    if (CallStateUtils::HasCall()) {
        FIRMWARE_LOGI("IsDeviceIdle: call is active, not idle");
        return false;
    }
    if (MusicStateUtils::IsMusicActive()) {
        FIRMWARE_LOGI("IsDeviceIdle: music is playing, not idle");
        return false;
    }
    FIRMWARE_LOGI("IsDeviceIdle: device is idle");
    return true;
}
} // namespace UpdateService
} // namespace OHOS
