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

#ifndef CHECK_MODE_H
#define CHECK_MODE_H

namespace OHOS::UpdateService {
enum class CheckMode {
    AUTO = 0,
    MANUL = 1,
    PUSH = 2,
    DEMO_REBOOT = 3,
    UPGRADE_COMPLETE = 4,
    DISABLE_COLL_ABILITY = 5,
    ENABLE_COLL_ABILITY = 6
};
}
#endif // CHECK_MODE_H
