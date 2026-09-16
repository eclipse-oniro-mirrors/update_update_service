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

#include "firmware_night_upgrade_alarm_manager.h"

#include "firmware_log.h"

namespace OHOS::UpdateService {
FirmwareNightUpgradeAlarmManager::FirmwareNightUpgradeAlarmManager()
{
    FIRMWARE_LOGI("FirmwareNightUpgradeAlarmManager empty");
}

FirmwareNightUpgradeAlarmManager::~FirmwareNightUpgradeAlarmManager()
{
}

void FirmwareNightUpgradeAlarmManager::SetNightUpgradeAlarm()
{
}

void FirmwareNightUpgradeAlarmManager::CancelNightUpgradeAlarm()
{
}

void FirmwareNightUpgradeAlarmManager::OnTimeChanged()
{
}

int64_t FirmwareNightUpgradeAlarmManager::GetNightUpgradeTime()
{
    return -1; // 夜间升级 feature 关闭时不会走到，返回无效值
}
} // namespace OHOS::UpdateService
