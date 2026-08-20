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

#include "system_ability_operator.h"

#include <iservice_registry.h>

#include "errors.h"
#include "common_event_support.h"
#include "system_ability_definition.h"

#include "update_log.h"

namespace OHOS {
namespace UpdateService {
bool SystemAbilityOperator::UpdateStartupPolicy(const std::vector<ScheduleTask> &scheduleTasks,
    const std::string &timeEventName)
{
    ENGINE_LOGI("UpdateStartupPolicy");
    constexpr uint64_t validMinInterval = 30;
    std::vector<SystemAbilityOnDemandEvent> abilityOnDemandEvents;
    for (const auto &task : scheduleTasks) {
        // 启停Policy中增加TIMED事件
        if (task.minDelayTime >= validMinInterval) {
            abilityOnDemandEvents.emplace_back(CreateTimedEvent(task.minDelayTime, timeEventName));
            ENGINE_LOGI("UpdateStartupPolicy update timed event : %{public}s, loop interval %{public}s",
                timeEventName.c_str(), std::to_string(task.minDelayTime).c_str());
        } else {
            ENGINE_LOGE("UpdateStartupPolicy : %{public}s, failure, invalid loop interval %{public}s",
                timeEventName.c_str(), std::to_string(task.minDelayTime).c_str());
            return false;
        }
    }

    return UpdateStartupOnDemandPolicy(abilityOnDemandEvents);
}

SystemAbilityOnDemandEvent SystemAbilityOperator::CreateTimedEvent(uint64_t nextStartDuration,
    const std::string &timedEventName)
{
    SystemAbilityOnDemandEvent timedEvent;
    timedEvent.eventId = OnDemandEventId::TIMED_EVENT;
    timedEvent.name = timedEventName;
    timedEvent.value = std::to_string(nextStartDuration);
    return timedEvent;
}

bool SystemAbilityOperator::UpdateStartupOnDemandPolicy(const std::vector<SystemAbilityOnDemandEvent> &events)
{
    ENGINE_LOGI("UpdateStartupOnDemandPolicy");
    auto samgr = GetSystemAbilityManager();
    if (samgr == nullptr) {
        ENGINE_LOGE("UpdateStartupOnDemandPolicy failure, caused by samgr object null");
        return false;
    }

    auto ret = samgr->UpdateOnDemandPolicy(UPDATE_DISTRIBUTED_SERVICE_ID, OnDemandPolicyType::START_POLICY, events);
    if (ret != ERR_OK) {
        ENGINE_LOGE("UpdateStartupOnDemandPolicy failure, caused by samgr interface call fail, error code %{public}d",
            ret);
        return false;
    }

    ENGINE_LOGD("UpdateStartupOnDemandPolicy success");
    return true;
}

bool SystemAbilityOperator::UnloadSystemAbility()
{
    ENGINE_LOGI("UnloadSystemAbility");
    auto samgr = GetSystemAbilityManager();
    if (samgr == nullptr) {
        ENGINE_LOGE("UnloadSystemAbility failure, caused by samgr object null!");
        return false;
    }

    int32_t ret = samgr->UnloadSystemAbility(UPDATE_DISTRIBUTED_SERVICE_ID);
    if (ret != ERR_OK) {
        ENGINE_LOGE("UnloadSystemAbility failure, caused by samgr interface call fail, error code %{public}d", ret);
        return false;
    }

    ENGINE_LOGD("UnloadSystemAbility success");
    return true;
}

std::vector<SystemAbilityOnDemandEvent> SystemAbilityOperator::GetStartupOnDemandPolicy()
{
    ENGINE_LOGI("GetStartupOnDemandPolicy");
    auto samgr = GetSystemAbilityManager();
    if (samgr == nullptr) {
        ENGINE_LOGE("GetStarrtupOnDemandPolicy failure, caused by samgr object null");
        return {};
    }

    std::vector<SystemAbilityOnDemandEvent> events;
    int32_t ret = samgr->GetOnDemandPolicy(UPDATE_DISTRIBUTED_SERVICE_ID, OnDemandPolicyType::START_POLICY, events);
    if (ret != ERR_OK) {
        ENGINE_LOGE("GetStartupOnDemandPolicy failure, caused by samgr interface call fail, error code %{public}d",
            ret);
        return {};
    }

    ENGINE_LOGD("GetStartupOnDemandPolicy success");
    return events;
}

inline sptr<ISystemAbilityManager> SystemAbilityOperator::GetSystemAbilityManager() const
{
    return SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
}
} // namespace UpdateService
} // namespace OHOS
