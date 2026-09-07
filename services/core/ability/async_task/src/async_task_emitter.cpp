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

#include "async_task_emitter.h"

#include <memory>

#include "update_log.h"

namespace OHOS::UpdateService {
AsyncTaskEmitter::AsyncTaskEmitter(const std::string &idetifier)
{
    idetifier_ = idetifier;
    ENGINE_LOGI("AsyncTaskEmitter constructor, idetifier %{public}s", idetifier_.c_str());
}

AsyncTaskEmitter::~AsyncTaskEmitter()
{
    ENGINE_LOGI("AsyncTaskEmitter destructor, idetifier %{public}s", idetifier_.c_str());
}

AsyncTaskEmitter &AsyncTaskEmitter::Emit(const AsyncTask &task)
{
    if (task == nullptr) {
        ENGINE_LOGE("Emit abort, null task, idetifier %{public}s", idetifier_.c_str());
        return *this;
    }

    asyncTaskHandles_.emplace_back(ffrt::submit_h(task));
    return *this;
}

AsyncTaskEmitter &AsyncTaskEmitter::Emit(const std::vector<AsyncTask> &tasks)
{
    if (tasks.empty()) {
        ENGINE_LOGE("Emit abort, empty tasks, idetifier %{public}s", idetifier_.c_str());
        return *this;
    }

    for (const auto &task : tasks) {
        Emit(task);
    }
    return *this;
}

void AsyncTaskEmitter::Wait()
{
    if (asyncTaskHandles_.empty()) {
        ENGINE_LOGD("Wait abort, no running task, idetifier %{public}s", idetifier_.c_str());
        return;
    }

    ENGINE_LOGI("Wait, idetifier %{public}s, running tasks size %{public}zu", idetifier_.c_str(),
        asyncTaskHandles_.size());
    ffrt::wait(asyncTaskHandles_);
    asyncTaskHandles_.clear();
}
} // namespace OHOS::UpdateService
