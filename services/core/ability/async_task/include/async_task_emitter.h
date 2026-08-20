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

#ifndef ASYNC_TASK_EMITTER_H
#define ASYNC_TASK_EMITTER_H

#include <functional>

#include <cpp/task.h>
#include <nocopyable.h>

namespace OHOS::UpdateService {
class AsyncTaskEmitter {
    DISALLOW_COPY_AND_MOVE(AsyncTaskEmitter);
    using AsyncTask = std::function<void()>;

public:
    explicit AsyncTaskEmitter(const std::string &idetifier = "");
    ~AsyncTaskEmitter();

    AsyncTaskEmitter &Emit(const AsyncTask &task);
    AsyncTaskEmitter &Emit(const std::vector<AsyncTask> &tasks);
    void Wait();

private:
    std::string idetifier_;
    std::vector<ffrt::dependence> asyncTaskHandles_;
};
} // namespace OHOS::UpdateService
#endif // ASYNC_TASK_EMITTER_H
