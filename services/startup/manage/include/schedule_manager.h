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

#ifndef SCHEDULE_MANAGER_H
#define SCHEDULE_MANAGER_H

#include <memory>

#include "access_manager.h"
#include "schedule_task.h"
#include "startup_schedule.h"

namespace OHOS {
namespace UpdateService {
class ScheduleManager final {
public:
    ScheduleManager(
        const std::shared_ptr<IAccessManager> &accessManager, const std::shared_ptr<IStartupSchedule> &startupSchedule);
    ~ScheduleManager();
    bool IdleCheck();
    bool Exit();

    bool TaskManage(std::vector<ScheduleTask> &scheduleTasks);
    bool TaskSchedule(const std::vector<ScheduleTask> &scheduleTasks);

private:
    std::shared_ptr<IAccessManager> accessManager_ = nullptr;
    std::shared_ptr<IStartupSchedule> startupSchedule_ = nullptr;
    std::vector<ScheduleTask> scheduleTask_;
};
} // namespace UpdateService
} // namespace OHOS
#endif // SCHEDULE_MANAGER_H