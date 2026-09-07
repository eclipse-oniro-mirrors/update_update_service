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

#ifndef SERIAL_ASYNC_TASK_EMITTER_H
#define SERIAL_ASYNC_TASK_EMITTER_H

#include <atomic>
#include <map>
#include <memory>
#include <cpp/mutex.h>
#include <cpp/task.h>
#include <nocopyable.h>
#include <singleton.h>
#include <cpp/queue.h>

namespace OHOS::UpdateService {
class SerialAsyncTaskQueueEmitter : public DelayedSingleton<SerialAsyncTaskQueueEmitter> {
    DECLARE_DELAYED_SINGLETON(SerialAsyncTaskQueueEmitter);
    using AsyncTask = std::function<void()>;

public:
    ffrt::task_handle Submit(const std::string &queueName, const AsyncTask &asyncTask,
        const ffrt::task_attr &attr = {});
    int32_t Cancel(const std::string &queueName, const ffrt::task_handle &handle);

private:
    void ClearUselessQueue(std::map<std::string, std::unique_ptr<ffrt::queue>> &queueMap);

private:
    ffrt::mutex mutex_;
    std::map<std::string, std::unique_ptr<ffrt::queue>> queueMap_{};

    std::atomic_int32_t submittedTaskCnt = 0;
};
} // namespace OHOS::UpdateService
#endif // SERIAL_ASYNC_TASK_EMITTER_H
