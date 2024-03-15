/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "constant.h"
#include "firmware_preferences_utils.h"
#include "startup_constant.h"
#include "system_ability_operator.h"
#include "time_utils.h"
#include "update_log.h"

namespace OHOS {
namespace UpdateEngine {
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
    ENGINE_LOGI("RegisterLooper");
}

void StartupSchedule::UnregisterLooper()
{
    ENGINE_LOGI("UnregisterLooper");
}

bool StartupSchedule::Schedule(const ScheduleTask &task)
{
    ENGINE_LOGI("Schedule");
    return true;
}

bool StartupSchedule::OnDemandSchedule(const std::vector<ScheduleTask> &tasks)
{
    ENGINE_LOGI("OnDemandSchedule");
    return true;
}
} // namespace UpdateEngine
} // namespace OHOS