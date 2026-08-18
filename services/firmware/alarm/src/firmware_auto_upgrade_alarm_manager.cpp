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

#include "firmware_auto_upgrade_alarm_manager.h"

#include "alarm_timer_utils.h"
#include "constant.h"
#include "dupdate_net_manager.h"
#include "firmware_event_manager.h"
#include "firmware_log.h"
#include "string_utils.h"
#include "time_utils.h"

namespace OHOS::UpdateService {
namespace {
constexpr int32_t TIMER_AVAILABLE_INTERVAL = 10; // 定时有效期为10s
}
FirmwareAutoUpgradeAlarmManager::FirmwareAutoUpgradeAlarmManager()
{
    FIRMWARE_LOGI("FirmwareAutoUpgradeAlarmManager");
}

FirmwareAutoUpgradeAlarmManager::~FirmwareAutoUpgradeAlarmManager()
{
    FIRMWARE_LOGI("~FirmwareAutoUpgradeAlarmManager");
    CancelAutoUpgradeAlarm();
}

void FirmwareAutoUpgradeAlarmManager::CheckAndHandleAutoUpgradeAlarm()
{
    int64_t autoUpgradeTime = preferencesUtil_->GetAutoUpgradeTime();
    FIRMWARE_LOGI("Firmware CheckAndHandleAutoUpgradeAlarm,alarmTime_: %{public}s",
        TimeUtils::GetPrintTimeStr(autoUpgradeTime).c_str());
    FirmwareEventManager::GetInstance()->HandleEvent(CommonEventType::AUTO_UPGRADE);
}

void FirmwareAutoUpgradeAlarmManager::RefreshAutoUpgradeAlarm()
{
    RegisterAutoUpgradeTime(AutoUpgradeTimeRefreshType::DURATION);
}

bool FirmwareAutoUpgradeAlarmManager::IsNeedIgnoreDuration(int32_t setDuration, int32_t currentDuration,
    int32_t minDuration, int32_t maxDuration)
{
    if (setDuration < minDuration) {
        return true;
    }
    setDuration = std::min(setDuration, maxDuration);
    if (setDuration == currentDuration) {
        return true;
    }
    return false;
}

void FirmwareAutoUpgradeAlarmManager::RefreshAutoUpgradeAlarm(CommonEventType event)
{
    FIRMWARE_LOGI("Firmware RefreshAutoUpgradeAlarmEvent,event: %{public}d", event);
    RegisterAutoUpgradeTime(AutoUpgradeTimeRefreshType::EVENT);
}

int64_t FirmwareAutoUpgradeAlarmManager::GetAutoUpgradeTime()
{
    return preferencesUtil_->GetAutoUpgradeTime();
}

int64_t FirmwareAutoUpgradeAlarmManager::GetEffectiveAutoUpgradeDuration()
{
    return Constant::DEFAULT_AUTO_UPGRADE_HOUR; // 单位：秒 默认24h
}

void FirmwareAutoUpgradeAlarmManager::RegisterAutoUpgradeTime(AutoUpgradeTimeRefreshType refreshType)
{
    // 设置下次搜包定时器
    int64_t duration = GetEffectiveAutoUpgradeDuration(); // 3600秒
    int64_t autoUpgradeTime = preferencesUtil_->GetAutoUpgradeTime();
    int64_t autoUpgradeTimeCalculateResult = autoUpgradeTime;
    CalculateAutoUpgradeTime(refreshType, autoUpgradeTimeCalculateResult, duration);
    if (autoUpgradeTime == autoUpgradeTimeCalculateResult && autoTimerId_ > 0) {
        FIRMWARE_LOGI("current AutoUpgradeTime %{public}s is active",
            TimeUtils::GetPrintTimeStr(autoUpgradeTime).c_str());
        return;
    }
    preferencesUtil_->SaveAutoUpgradeTime(autoUpgradeTimeCalculateResult);
    UnregisterAlarmTimer(autoTimerId_);
    autoTimerId_ =
        AlarmTimerUtils::RegisterAlarm(autoUpgradeTimeCalculateResult, [this]() { CheckAndHandleAutoUpgradeAlarm(); });
    FIRMWARE_LOGI("Firmware RegisterAutoUpgradeTime,alarmTime_: %{public}s",
        TimeUtils::GetPrintTimeStr(autoUpgradeTimeCalculateResult).c_str());
}

void FirmwareAutoUpgradeAlarmManager::CalculateAutoUpgradeTime(AutoUpgradeTimeRefreshType refreshType,
    int64_t &autoUpgradeTime, int64_t duration)
{
    FIRMWARE_LOGI("CalculateAutoUpgradeTime autoUpgradeTime: %{public}s duration: %{public}s",
        std::to_string(autoUpgradeTime).c_str(), std::to_string(duration).c_str());
    constexpr int64_t randomOffPeakTime = 0.5 * Constant::ONE_HOUR_SECONDS;
    int64_t currentTime = TimeUtils::GetTimestamp();
    switch (refreshType) {
        case AutoUpgradeTimeRefreshType::DURATION:
            autoUpgradeTime =
                currentTime + duration - randomOffPeakTime + TimeUtils::GetRandTime(0, Constant::ONE_HOUR_SECONDS);
            break;
        case AutoUpgradeTimeRefreshType::EVENT:
            HandleNetChange(currentTime, autoUpgradeTime, duration);
            break;
        case AutoUpgradeTimeRefreshType::RAND:
            autoUpgradeTime =
                currentTime + TimeUtils::GetRandTime(Constant::FIVE_MINUTES_SECONDS, Constant::ONE_HOUR_SECONDS);
            break;
        default:
            break;
    }
}

void FirmwareAutoUpgradeAlarmManager::HandleNetChange(int64_t currentTime, int64_t &autoUpgradeTime, int64_t duration)
{
    int64_t lastRegisterTime = (autoUpgradeTime != Constant::DEFAULT_UPGRADE_TIME) ? (autoUpgradeTime - duration) : 0;
    FIRMWARE_LOGI("Firmware HandleNetChange,autoUpgradeTime: %{public}s,lastRegisterTime: %{public}s",
        TimeUtils::GetPrintTimeStr(autoUpgradeTime).c_str(), TimeUtils::GetPrintTimeStr(lastRegisterTime).c_str());

    if (currentTime >= autoUpgradeTime || currentTime < lastRegisterTime) {
        autoUpgradeTime = currentTime + TIMER_AVAILABLE_INTERVAL;
    }
}

void FirmwareAutoUpgradeAlarmManager::CancelAutoUpgradeAlarm()
{
    ENGINE_LOGI("CancelAutoUpgradeAlarm");
    UnregisterAlarmTimer(autoTimerId_);
}

void FirmwareAutoUpgradeAlarmManager::UnregisterAlarmTimer(uint64_t &timerId)
{
    if (timerId > 0) {
        AlarmTimerUtils::UnregisterAlarm(timerId);
    }
    timerId = 0;
}
} // namespace OHOS::UpdateService