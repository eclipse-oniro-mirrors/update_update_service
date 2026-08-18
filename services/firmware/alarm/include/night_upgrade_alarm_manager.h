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

#ifndef NIGHT_UPGRADE_ALARM_MANAGER_H
#define NIGHT_UPGRADE_ALARM_MANAGER_H

#include <cstdint>
#include <map>

#include "singleton.h"

#include "alarm_timer_utils.h"
#include "constant.h"
#include "night_upgrade_priority.h"
#include "update_define.h"

namespace OHOS::UpdateService {
using Callback = AlarmTimerCallback;

class NightUpgradeAlarmManager final : public DelayedSingleton<NightUpgradeAlarmManager> {
    DECLARE_DELAYED_SINGLETON(NightUpgradeAlarmManager);

public:
    void RegisterNightUpgradeTask(const NightUpgradeInfo &nightUpgradeInfo, const Callback &callback);
    void UnregisterNightUpgradeTask(NightUpgradePriority nightUpgradePriority);
    bool IsRegisteredNightUpgradeTask(NightUpgradePriority nightUpgradePriority);
    int64_t GetNightUpgradeTime();

private:
    bool IsIllegalNightUpgradeTime(int64_t currentTime);
    void InnerRegisterNightUpgradeEvent();
    void InnerUnregisterNightUpgradeEvent();
    void HandleNightUpgradeAlarm();
    bool IsRegisteredNightUpgradeTask(const NightUpgradeInfo &nightUpgradeInfo);
    int64_t CalculateUpgradeTime(int64_t currentTime);

private:
    struct CompareClass {
    public:
        bool operator () (const NightUpgradeInfo &t1, const NightUpgradeInfo &t2) const
        {
            if (t1.isNeedReboot == t2.isNeedReboot) {
                return CAST_INT(t1.priority) < CAST_INT(t2.priority);
            }

            return !t1.isNeedReboot;
        }
    };

private:
    void HandleNightUpgrade(std::map<NightUpgradeInfo, Callback, CompareClass> nightUpgradeAlarmMap);

private:
    std::map<NightUpgradeInfo, Callback, CompareClass> nightUpgradeAlarmMap_;
    std::recursive_mutex nightUpgradeAlarmLock_;
    int64_t nightUpgradeTime_ = Constant::DEFAULT_UPGRADE_TIME;
    uint64_t nightTimerId_ = 0;
};
} // namespace OHOS::UpdateService
#endif // NIGHT_UPGRADE_ALARM_MANAGER_H