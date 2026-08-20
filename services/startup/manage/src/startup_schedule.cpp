/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "startup_schedule.h"

#include "service_control.h"

#include "alarm_timer_utils.h"
#include "constant.h"
#include "firmware_preferences_utils.h"
#include "startup_constant.h"
#include "system_ability_operator.h"
#include "time_utils.h"
#include "update_log.h"

namespace OHOS {
namespace UpdateService {
constexpr uint64_t STARTUP_LOOPER_INTERVAL = 180; // 动态启停定时器轮询周期
StartupSchedule::StartupSchedule()
{
    ENGINE_LOGD("StartupSchedule constructor");
}

StartupSchedule::~StartupSchedule()
{
    ENGINE_LOGD("StartupSchedule deConstructor");
}

void StartupSchedule::RegisterLooper(const ScheduleLooper &looper)
{
    UnregisterLooper();
    ENGINE_LOGI("RegisterLooper");
    int64_t startTime = static_cast<int64_t>(AlarmTimerUtils::GetSystemBootTime()) +
     static_cast<int64_t>(STARTUP_LOOPER_INTERVAL) * Constant::MILLESECONDS;
    looperTimerId_ = AlarmTimerUtils::RegisterRepeatAlarm(startTime, STARTUP_LOOPER_INTERVAL, [=]() { looper(); });
}

void StartupSchedule::UnregisterLooper()
{
    ENGINE_LOGI("UnregisterLooper");
    if (looperTimerId_ > 0) {
        AlarmTimerUtils::UnregisterAlarm(looperTimerId_);
    }
    looperTimerId_ = 0;
}

bool StartupSchedule::Schedule(const ScheduleTask &task)
{
    ENGINE_LOGI("Schedule next SA start time is %{public}s",
        TimeUtils::GetPrintTimeStr(TimeUtils::GetTimestamp() + task.minDelayTime).c_str());
    uint64_t scheduleTime = task.minDelayTime * Startup::ONE_SECOND_MILLISECONDS;

    // 由于SaMgr暂不支持记录启动原因，因此临时将启动原因写入SP文件中
    DelayedSingleton<FirmwarePreferencesUtil>::GetInstance()->SaveInt(Constant::PROCESS_RESTART_REASON,
        CAST_INT(task.startupReason));

    int32_t ret = StartServiceByTimer(Startup::UPDATER_SA_NAME.c_str(), scheduleTime);
    ENGINE_LOGI("StartServiceByTimer finish, ret is %{public}d", ret);
    return ret == 0;
}

bool StartupSchedule::OnDemandSchedule(const std::vector<ScheduleTask> &tasks)
{
    if (tasks.empty()) {
        ENGINE_LOGE("scheduleTasks is null");
        return false;
    }
    for (const auto &task : tasks) {
        ENGINE_LOGI("OnDemandSchedule task %{public}s", task.ToString().c_str());
    }
    bool isSuccess = SystemAbilityOperator().UpdateStartupPolicy(tasks, "awakeloopevent");
    if (isSuccess) {
        ENGINE_LOGI("OnDemandSchedule %{public}s", isSuccess ? "success" : "failure");
        return isSuccess;
    }

    isSuccess = SystemAbilityOperator().UpdateStartupPolicy(tasks, "loopevent");
    ENGINE_LOGI("OnDemandSchedule 2nd %{public}s", isSuccess ? "success" : "failure");
    return isSuccess;
}
} // namespace UpdateService
} // namespace OHOS