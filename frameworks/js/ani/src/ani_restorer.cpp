/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ani_restorer.h"

#include "ani_common_conveter.h"
#include "update_service_kits.h"

namespace OHOS::UpdateService {
void AniRestorer::FactoryResetSync()
{
    BusinessError error;
    const int32_t ret = UpdateServiceKits::GetInstance().FactoryReset(error);
    SetError(ret, "factoryReset", error);
}

void AniRestorer::ForceFactoryResetSync()
{
    BusinessError error;
    const int32_t ret = UpdateServiceKits::GetInstance().ForceFactoryReset(error);
    SetError(ret, "forceFactoryReset", error);
}

void AniRestorer::DeepFactoryResetSync(const ohos::update::FactoryResetStrategy &factoryResetStrategy)
{
    BusinessError error;
    const int32_t ret = UpdateServiceKits::GetInstance().DeepFactoryReset(
        AniCommonConverter::Converter(factoryResetStrategy), error);
    SetError(ret, "deepFactoryResetSync", error);
}

ohos::update::FactoryResetInfo AniRestorer::GetDeepFactoryResetInfoSync(
    const ohos::update::FactoryResetStrategy &factoryResetStrategy)
{
    BusinessError error;
    FactoryResetInfo factoryResetInfo;
    const int32_t ret = UpdateServiceKits::GetInstance().GetDeepFactoryResetInfo(
        AniCommonConverter::Converter(factoryResetStrategy), factoryResetInfo, error);
    SetError(ret, "getDeepFactoryResetInfoSync", error);
    return AniCommonConverter::Converter(factoryResetInfo);
}

std::string AniRestorer::GetPermissionName()
{
    return "ohos.permission.FACTORY_RESET";
}
} // namespace OHOS::UpdateService
