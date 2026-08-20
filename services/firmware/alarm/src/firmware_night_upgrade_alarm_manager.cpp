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

#include "firmware_night_upgrade_alarm_manager.h"

#include "alarm_timer_utils.h"
#include "firmware_event_manager.h"
#include "firmware_log.h"
#include "firmware_task_operator.h"
#include "night_upgrade_priority.h"
#include "time_utils.h"

namespace OHOS::UpdateService {
FirmwareNightUpgradeAlarmManager::FirmwareNightUpgradeAlarmManager()
{
    FIRMWARE_LOGI("FirmwareNightUpgradeAlarmManager");
}

FirmwareNightUpgradeAlarmManager::~FirmwareNightUpgradeAlarmManager()
{
    FIRMWARE_LOGI("~FirmwareNightUpgradeAlarmManager");
    CancelNightUpgradeAlarm();
}

void FirmwareNightUpgradeAlarmManager::SetNightUpgradeAlarm()
{
    FIRMWARE_LOGI("SetNightUpgradeAlarm");
    NightUpgradeInfo nightUpgradeInfo;
    nightUpgradeInfo.priority = NightUpgradePriority::FIRMWARE;
    nightUpgradeInfo.isNeedReboot = true;
    NightUpgradeAlarmManager::GetInstance()->RegisterNightUpgradeTask(nightUpgradeInfo,
        [this]() { OnNightUpgradeTimeUp(); });
}

void FirmwareNightUpgradeAlarmManager::CancelNightUpgradeAlarm()
{
    FIRMWARE_LOGI("CancelNightUpgradeAlarm");
    DelayedSingleton<NightUpgradeAlarmManager>::GetInstance()->UnregisterNightUpgradeTask(
        NightUpgradePriority::FIRMWARE);
}

void FirmwareNightUpgradeAlarmManager::OnTimeChanged()
{
    FIRMWARE_LOGI("OnTimeChanged");
    if (DelayedSingleton<NightUpgradeAlarmManager>::GetInstance()->IsRegisteredNightUpgradeTask(
        NightUpgradePriority::FIRMWARE)) {
        SetNightUpgradeAlarm();
    }
}

void FirmwareNightUpgradeAlarmManager::OnNightUpgradeTimeUp()
{
    DelayedSingleton<FirmwareEventManager>::GetInstance()->HandleEvent(CommonEventType::NIGHT_UPGRADE);
}

int64_t FirmwareNightUpgradeAlarmManager::GetNightUpgradeTime()
{
    return DelayedSingleton<NightUpgradeAlarmManager>::GetInstance()->GetNightUpgradeTime();
}
} // namespace OHOS::UpdateService