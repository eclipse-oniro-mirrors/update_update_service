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

#include "night_upgrade_alarm_manager.h"

#include "constant.h"
#include "string_utils.h"
#include "time_utils.h"
#include "update_log.h"
#include "night_upgrade_priority.h"

namespace OHOS::UpdateService {
namespace {
constexpr int32_t HOUR_TWO = 2;
constexpr int32_t HOUR_THREE = 3;
}
NightUpgradeAlarmManager::NightUpgradeAlarmManager()
{
    ENGINE_LOGI("NightUpgradeAlarmManager");
}

NightUpgradeAlarmManager::~NightUpgradeAlarmManager()
{
    ENGINE_LOGI("~NightUpgradeAlarmManager");
}

void NightUpgradeAlarmManager::RegisterNightUpgradeTask(const NightUpgradeInfo &nightUpgradeInfo,
    const Callback &callback)
{
    std::lock_guard<std::recursive_mutex> lock(nightUpgradeAlarmLock_);
    ENGINE_LOGI("RegisterNextNightUpgradeAlarm, nightUpgradePriority is %{public}d",
        CAST_INT(nightUpgradeInfo.priority));
    if (!IsRegisteredNightUpgradeTask(nightUpgradeInfo)) {
        nightUpgradeAlarmMap_[nightUpgradeInfo] = callback;
    }
    InnerRegisterNightUpgradeEvent();
}

void NightUpgradeAlarmManager::UnregisterNightUpgradeTask(NightUpgradePriority nightUpgradePriority)
{
    std::lock_guard<std::recursive_mutex> lock(nightUpgradeAlarmLock_);
    ENGINE_LOGI("UnregisterNextNightUpgradeAlarm, nightUpgradePriority is %{public}d", CAST_INT(nightUpgradePriority));
    auto iter = std::find_if(nightUpgradeAlarmMap_.begin(), nightUpgradeAlarmMap_.end(),
        [&](const std::pair<NightUpgradeInfo, Callback> &pair) { return pair.first.priority == nightUpgradePriority; });
    if (iter != nightUpgradeAlarmMap_.end()) {
        nightUpgradeAlarmMap_.erase(iter);
    }

    if (nightUpgradeAlarmMap_.empty()) {
        ENGINE_LOGE("nightUpgradeAlarmMap is empty");
        InnerUnregisterNightUpgradeEvent();
        return;
    }
}

bool NightUpgradeAlarmManager::IsRegisteredNightUpgradeTask(NightUpgradePriority nightUpgradePriority)
{
    std::lock_guard<std::recursive_mutex> lock(nightUpgradeAlarmLock_);
    auto iter = std::find_if(nightUpgradeAlarmMap_.begin(), nightUpgradeAlarmMap_.end(),
        [&](const std::pair<NightUpgradeInfo, Callback> &pair) { return pair.first.priority == nightUpgradePriority; });
    return iter != nightUpgradeAlarmMap_.end() && nightTimerId_ > 0;
}

bool NightUpgradeAlarmManager::IsRegisteredNightUpgradeTask(const NightUpgradeInfo &nightUpgradeInfo)
{
    std::lock_guard<std::recursive_mutex> lock(nightUpgradeAlarmLock_);
    return nightUpgradeAlarmMap_.count(nightUpgradeInfo) && nightTimerId_ > 0;
}

bool NightUpgradeAlarmManager::IsIllegalNightUpgradeTime(int64_t currentTime)
{
    return nightUpgradeTime_ == Constant::DEFAULT_UPGRADE_TIME || nightUpgradeTime_ < currentTime ||
        nightUpgradeTime_ - currentTime > Constant::ONE_DAY_SECOND;
}

void NightUpgradeAlarmManager::InnerRegisterNightUpgradeEvent()
{
    int64_t currentTime = TimeUtils::GetTimestamp();
    ENGINE_LOGI("RegisterNightUpgradeEvent, currentTime  str %{public}s",
        TimeUtils::GetPrintTimeStr(currentTime).c_str());
    if (!IsIllegalNightUpgradeTime(currentTime)) {
        ENGINE_LOGI("CheckNightUpgradeAlarm night upgrade time str %{public}s",
            TimeUtils::GetPrintTimeStr(nightUpgradeTime_).c_str());
        return;
    }
    InnerUnregisterNightUpgradeEvent();
    nightUpgradeTime_ = CalculateUpgradeTime(currentTime);
    ENGINE_LOGI("RegisterNightUpgradeEvent, nightTime  str %{public}s",
        TimeUtils::GetPrintTimeStr(nightUpgradeTime_).c_str());
    ENGINE_LOGI("CheckNightUpgradeAlarm night upgrade time str %{public}s",
        TimeUtils::GetPrintTimeStr(nightUpgradeTime_).c_str());
    nightTimerId_ = AlarmTimerUtils::RegisterAlarm(nightUpgradeTime_, [this]() { HandleNightUpgradeAlarm(); });
}

void NightUpgradeAlarmManager::InnerUnregisterNightUpgradeEvent()
{
    if (nightTimerId_ > 0) {
        AlarmTimerUtils::UnregisterAlarm(nightTimerId_);
    }
    nightTimerId_ = 0;
    nightUpgradeTime_ = Constant::DEFAULT_UPGRADE_TIME;
}

void NightUpgradeAlarmManager::HandleNightUpgradeAlarm()
{
    std::lock_guard<std::recursive_mutex> lock(nightUpgradeAlarmLock_);
    ENGINE_LOGI("HandleNightUpgradeAlarm, nightTime str %{public}s",
        TimeUtils::GetPrintTimeStr(nightUpgradeTime_).c_str());
    ENGINE_LOGI("night upgrade time is up");
    nightUpgradeTime_ = Constant::DEFAULT_UPGRADE_TIME;
    nightTimerId_ = 0;
    if (nightUpgradeAlarmMap_.empty()) {
        ENGINE_LOGE("HandleNightUpgradeAlarm, nightUpgradeTask is empty");
        return;
    }
    HandleNightUpgrade(nightUpgradeAlarmMap_);
}

void NightUpgradeAlarmManager::HandleNightUpgrade(
    std::map<NightUpgradeInfo, Callback, CompareClass> nightUpgradeAlarmMap)
{
    for (const auto &[key, callback] : nightUpgradeAlarmMap) {
        callback();
        if (key.isNeedReboot) {
            break;
        }
    }
}

int64_t NightUpgradeAlarmManager::GetNightUpgradeTime()
{
    return nightUpgradeTime_;
}

int64_t NightUpgradeAlarmManager::CalculateUpgradeTime(int64_t currentTime)
{
    int64_t upgradeTime;
    tm *currentTm = localtime(&currentTime);
    if (currentTm == nullptr) {
        ENGINE_LOGE("currentTm is NULL");
        return 0;
    }
    if (currentTm->tm_hour < HOUR_TWO) {
        currentTm->tm_hour = HOUR_TWO;
        int randomTime = TimeUtils::GetRandTime(0, Constant::ONE_HOUR_MINUTES);
        currentTm->tm_min = randomTime;
        upgradeTime = static_cast<int64_t>(mktime(currentTm));
    } else if (currentTm->tm_hour < HOUR_THREE) {
        currentTm->tm_hour = HOUR_THREE;
        int randomTime = TimeUtils::GetRandTime(0, Constant::ONE_HOUR_MINUTES);
        currentTm->tm_min = randomTime;
        upgradeTime = static_cast<int64_t>(mktime(currentTm));
    } else {
        currentTm->tm_hour = 0;
        currentTm->tm_min = 0;
        time_t zeroTime = mktime(currentTm);
        int randTime = TimeUtils::GetRandTime(0, Constant::ONE_HOUR_MINUTES);

        upgradeTime = static_cast<int64_t>(zeroTime) + Constant::ONE_DAY_SECOND +
            HOUR_TWO * Constant::ONE_HOUR_MINUTES * Constant::ONE_MINUTE_SECONDS +
            randTime * Constant::ONE_MINUTE_SECONDS;
    }
    ENGINE_LOGI("CalculateNightUpgradeTime, upgrade time str %{public}s",
        TimeUtils::GetPrintTimeStr(upgradeTime).c_str());
    return upgradeTime;
}
} // namespace OHOS::UpdateService
