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

#include "serial_async_task_emitter.h"

#include <mutex>

#include "update_log.h"

namespace OHOS::UpdateService {
constexpr int32_t TASK_CLEAN_CYCLE = 32;

SerialAsyncTaskQueueEmitter::SerialAsyncTaskQueueEmitter()
{
    ENGINE_LOGI("SerialAsyncTaskQueueEmitter");
}

SerialAsyncTaskQueueEmitter::~SerialAsyncTaskQueueEmitter()
{
    ENGINE_LOGI("~SerialAsyncTaskQueueEmitter");
}

ffrt::task_handle SerialAsyncTaskQueueEmitter::Submit(const std::string &queueName, const AsyncTask &asyncTask,
    const ffrt::task_attr &attr)
{
    std::lock_guard<ffrt::mutex> lockGuard(mutex_);
    submittedTaskCnt.fetch_add(1);
    if (submittedTaskCnt.load() % TASK_CLEAN_CYCLE == 0) {
        submittedTaskCnt = 0;
        ClearUselessQueue(queueMap_);
    }

    if (!queueMap_.count(queueName) || queueMap_.at(queueName) == nullptr) {
        queueMap_.emplace(queueName, std::make_unique<ffrt::queue>(ffrt::queue_serial, queueName.c_str(),
            ffrt::queue_attr().qos(ffrt::qos_utility)));
        ENGINE_LOGI("Submit, create serial task queue %{public}s", queueName.c_str());
    }

    std::unique_ptr<ffrt::queue> &serialQueue = queueMap_.at(queueName);
    if (serialQueue == nullptr) {
        return nullptr;
    }
    return serialQueue->submit_h(asyncTask, attr);
}

int32_t SerialAsyncTaskQueueEmitter::Cancel(const std::string &queueName, const ffrt::task_handle &handle)
{
    if (handle == nullptr) {
        return -1;
    }

    std::lock_guard<ffrt::mutex> lockGuard(mutex_);
    if (!queueMap_.count(queueName)) {
        return -1;
    }

    const auto &serialQueue = queueMap_.at(queueName);
    if (serialQueue == nullptr) {
        queueMap_.erase(queueName);
        return -1;
    }
    return serialQueue->cancel(handle);
}

void SerialAsyncTaskQueueEmitter::ClearUselessQueue(std::map<std::string, std::unique_ptr<ffrt::queue>> &queueMap)
{
    if (queueMap.empty() || queueMap.size() < TASK_CLEAN_CYCLE) {
        return;
    }

    for (auto it = queueMap.begin(); it != queueMap.end();) {
        const auto &queue = it->second;
        if (queue == nullptr) {
            ENGINE_LOGE("ClearUselessQueue, queue is nullptr");
            it = queueMap.erase(it);
        } else if (queue->get_task_cnt() == 0) {
            ENGINE_LOGI("ClearUselessQueue, queue name %{public}s", it->first.c_str());
            it = queueMap.erase(it);
        } else {
            ++it;
        }
    }
}
} // namespace OHOS::UpdateService
