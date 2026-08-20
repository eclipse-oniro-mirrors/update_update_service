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

#ifndef FIRMWARE_AUTO_UPGRADE_ALARM_MANAGER_H
#define FIRMWARE_AUTO_UPGRADE_ALARM_MANAGER_H

#include "singleton.h"

#include "constant.h"
#include "firmware_common.h"
#include "firmware_preferences_utils.h"

namespace OHOS::UpdateService {
enum class AutoUpgradeTimeRefreshType {
    VERIFY = 0,
    DURATION,
    EVENT,
    RAND,
    DELAY_TIME
};

class FirmwareAutoUpgradeAlarmManager : public DelayedSingleton<FirmwareAutoUpgradeAlarmManager> {
    DECLARE_DELAYED_SINGLETON(FirmwareAutoUpgradeAlarmManager);

public:
    void RefreshAutoUpgradeAlarm();
    void RefreshAutoUpgradeAlarm(CommonEventType event);
    int64_t GetAutoUpgradeTime();
    int64_t GetEffectiveAutoUpgradeDuration();

private:
    void CheckAndHandleAutoUpgradeAlarm();
    void RegisterAutoUpgradeTime(AutoUpgradeTimeRefreshType refreshType);
    void CalculateAutoUpgradeTime(AutoUpgradeTimeRefreshType refreshType, int64_t &autoUpgradeTime, int64_t duration);
    void HandleNetChange(int64_t currentTime, int64_t &autoUpgradeTime, int64_t duration);
    void CancelAutoUpgradeAlarm();
    void UnregisterAlarmTimer(uint64_t &timerId);
    bool IsNeedIgnoreDuration(int32_t setDuration, int32_t currentDuration, int32_t minDuration, int32_t maxDuration);

private:
    uint64_t autoTimerId_ = 0;
    std::shared_ptr<FirmwarePreferencesUtil> preferencesUtil_ = FirmwarePreferencesUtil::GetInstance();
};
} // namespace OHOS::UpdateService
#endif // FIRMWARE_AUTO_UPGRADE_ALARM_MANAGER_H