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

#include "schedule_manager.h"

#include "constant.h"
#include "schedule_config.h"
#include "update_log.h"
#include "network_type.h"

namespace OHOS {
namespace UpdateService {
ScheduleManager::ScheduleManager(const std::shared_ptr<IAccessManager> &accessManager,
    const std::shared_ptr<IStartupSchedule> &startupSchedule)
{
    ENGINE_LOGD("ScheduleManager constructor");
    accessManager_ = accessManager;
    startupSchedule_ = startupSchedule;
}

ScheduleManager::~ScheduleManager()
{
    ENGINE_LOGD("ScheduleManager deConstructor");
}

bool ScheduleManager::IdleCheck()
{
    if (accessManager_ != nullptr && !accessManager_->IsIdle()) {
        return false;
    }
    ENGINE_LOGI("IdleCheck true");
    TaskManage(scheduleTask_);
    return true;
}

bool ScheduleManager::Exit()
{
    if (accessManager_ != nullptr && !accessManager_->Exit()) {
        return false;
    }
    return TaskSchedule(scheduleTask_);
}

bool ScheduleManager::TaskManage(std::vector<ScheduleTask> &scheduleTasks)
{
    // 函数入口的scheduleTasks本质上是scheduleTasks_,为了防止出现脏数据，需要重置下
    scheduleTasks.clear();
    if (accessManager_ == nullptr) {
        ENGINE_LOGE("TaskManage accessManager is null, return idle");
        return true;
    }

    std::vector<ScheduleTask> tasks = accessManager_->GetScheduleTasks();
    // 筛选等待网络的任务，所需网络类型汇总后从总任务列表清除
    ScheduleTask netTask;
    for (auto iter = tasks.begin(); iter != tasks.end();) {
        ENGINE_LOGI("TaskManage %{public}s", iter->ToString().c_str());
        if (iter->netType != NetType::NO_NET) { // 筛选等待网络的任务，所需网络类型汇总后从总任务列表清除
            netTask.netType = static_cast<NetType>(CAST_UINT(netTask.netType) | CAST_UINT(iter->netType));
            iter = tasks.erase(iter);
        } else {
            ++iter;
        }
    }
    if (netTask.IsValidNetTask()) {
        scheduleTasks.emplace_back(netTask);
    }

    std::sort(tasks.begin(), tasks.end());
    ScheduleTask timedTask;
    if (tasks.empty()) {
        // 过滤后任务列表为空，取周期拉起时间兜底&打异常日志
        timedTask.minDelayTime = Constant::DEFAULT_PULLUP_INTERVAL;
        ENGINE_LOGE("TaskManage time task is empty, use default pull up interval %{public}s",
            std::to_string(timedTask.minDelayTime).c_str());
    } else {
        // 过滤后任务列表不为空，取最近任务时间
        timedTask.minDelayTime = tasks.front().minDelayTime;
    }
    scheduleTasks.emplace_back(timedTask);

    // 判断是否可退出，当最近任务大于SA保活阈值时才可以退出
    if (timedTask.minDelayTime > Constant::KEEP_ALIVE_THRESHOLD) {
        ENGINE_LOGI("TaskManage next pull up interval is %{public}s", std::to_string(timedTask.minDelayTime).c_str());
        return true;
    }
    ENGINE_LOGI("TaskManage task will be triggered after %{public}s seconds, keep sa alive",
        std::to_string(timedTask.minDelayTime).c_str());
    return false;
}

bool ScheduleManager::TaskSchedule(const std::vector<ScheduleTask> &scheduleTasks)
{
    if (startupSchedule_ == nullptr) {
        ENGINE_LOGE("TaskSchedule startupSchedule null, return false");
        return false;
    }
    return startupSchedule_->OnDemandSchedule(scheduleTasks);
}
} // namespace UpdateService
} // namespace OHOS