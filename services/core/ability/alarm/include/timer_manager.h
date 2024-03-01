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

#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <map>

#include "singleton.h"
#include "timer.h"

namespace OHOS {
namespace UpdateEngine {
enum class EventType {
    STARTUP_IDLE_LOOPER_EVENT = 1
};

class TimerManager : public DelayedSingleton<TimerManager>, public std::enable_shared_from_this<TimerManager> {
    DECLARE_DELAYED_SINGLETON(TimerManager);

public:
    bool RegisterLooperEvent(EventType eventType, int64_t looperInterval,
        const OHOS::Utils::Timer::TimerCallback &callback); // looperInterval：循环间隔, 单位：秒
    void UnregisterLooperEvent(EventType eventType);

private:
    std::map<EventType, uint32_t> registeredTimerIdMap_;
    std::shared_ptr<OHOS::Utils::Timer> timer_ = nullptr;
    std::mutex mutex_;
};
} // namespace UpdateEngine
} // namespace OHOS
#endif // TIMER_MANAGER_H