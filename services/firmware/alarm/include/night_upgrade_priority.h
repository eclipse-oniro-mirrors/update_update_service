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

#ifndef NIGHT_UPGRADE_PRIORITY_H
#define NIGHT_UPGRADE_PRIORITY_H
namespace OHOS::UpdateService {
// 夜间自动升级优先级，值越小优先级越高
enum class NightUpgradePriority {
    FIRMWARE = 0,
    PARAM = 1,
    LARGE_MODEL = 2,
    MERGE = 3,
    DEFAULT
};

struct NightUpgradeInfo {
    NightUpgradePriority priority = NightUpgradePriority::DEFAULT;
    bool isNeedReboot = true;
};
} // namespace OHOS::UpdateEngine
#endif // NIGHT_UPGRADE_PRIORITY_H
