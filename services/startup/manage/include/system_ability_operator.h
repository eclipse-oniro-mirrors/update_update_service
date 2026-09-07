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

#ifndef SYSTEM_ABILITY_OPERATOR_H
#define SYSTEM_ABILITY_OPERATOR_H

#include <if_system_ability_manager.h>
#include "refbase.h"
#include "system_ability_on_demand_event.h"

#include "schedule_task.h"

namespace OHOS {
namespace UpdateService {
class ISystemAbilityOperator {
public:
    virtual ~ISystemAbilityOperator() = default;
    virtual bool UpdateStartupPolicy(const std::vector<ScheduleTask> &scheduleTasks,
        const std::string &timeEventName) = 0;
    virtual bool UnloadSystemAbility() = 0;
    virtual std::vector<SystemAbilityOnDemandEvent> GetStartupOnDemandPolicy() = 0;
};

class SystemAbilityOperator final : public ISystemAbilityOperator {
public:
    bool UpdateStartupPolicy(const std::vector<ScheduleTask> &scheduleTasks, const std::string &timeEventName) final;
    bool UnloadSystemAbility() final;
    std::vector<SystemAbilityOnDemandEvent> GetStartupOnDemandPolicy() final;

private:
    SystemAbilityOnDemandEvent CreateTimedEvent(uint64_t nextStartDuration, const std::string &timedEventName);
    bool UpdateStartupOnDemandPolicy(const std::vector<SystemAbilityOnDemandEvent> &events);
    sptr<ISystemAbilityManager> GetSystemAbilityManager() const;
};
} // namespace UpdateService
} // namespace OHOS
#endif // SYSTEM_ABILITY_OPERATOR_H