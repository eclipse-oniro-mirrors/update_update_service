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

#include "default_access.h"

#include "update_log.h"

namespace OHOS {
namespace UpdateService {
DefaultAccess::DefaultAccess()
{
    ENGINE_LOGD("DefaultAccess constructor");
}

DefaultAccess::~DefaultAccess()
{
    ENGINE_LOGD("DefaultAccess deConstructor");
}

void DefaultAccess::Init(StartupReason startupReason)
{
    ENGINE_LOGI("DefaultAccess Init");
}

bool DefaultAccess::IsIdle()
{
    return true;
}

std::vector<ScheduleTask> DefaultAccess::GetScheduleTasks()
{
    return {};
}

bool DefaultAccess::Exit()
{
    return true;
}
} // namespace UpdateService
} // namespace OHOS