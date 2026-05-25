/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ANI_RESTORER_H
#define ANI_RESTORER_H

#include "ani_base_updater.h"
#include "factory_reset_info.h"
#include "factory_reset_strategy.h"

namespace OHOS::UpdateService {
class AniRestorer final : public AniBaseUpdater {
public:
    explicit AniRestorer() = default;
    ~AniRestorer() override = default;

    void FactoryResetSync();
    void ForceFactoryResetSync();
    void DeepFactoryResetSync(const ohos::update::FactoryResetStrategy &factoryResetStrategy);
    ohos::update::FactoryResetInfo GetDeepFactoryResetInfoSync(
        const ohos::update::FactoryResetStrategy &factoryResetStrategy);

protected:
    std::string GetPermissionName() override;
};
} // namespace OHOS::UpdateService
#endif // ANI_RESTORER_H
