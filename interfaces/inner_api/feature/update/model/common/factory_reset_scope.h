/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef UPDATE_SERVICE_FACTORY_RESET_SCOPE_H
#define UPDATE_SERVICE_FACTORY_RESET_SCOPE_H

namespace OHOS::UpdateService {
enum class FactoryResetScope {
    DATA = 1,
    DATA_AND_OS = 2
};
} // namespace OHOS::UpdateService
#endif // UPDATE_SERVICE_FACTORY_RESET_SCOPE_H
