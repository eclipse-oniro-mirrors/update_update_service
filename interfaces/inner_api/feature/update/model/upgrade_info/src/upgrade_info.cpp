/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "encrypt_utils.h"
#include "upgrade_info.h"
#include "update_define.h"

namespace OHOS::UpdateEngine {
std::string UpgradeInfo::ToString() const
{
    std::string output = "upgradeApp:" + upgradeApp;
    output += ",businessType(vender:" + businessType.vendor;
    output += ",subType:" + std::to_string(CAST_INT(businessType.subType));
    output += "),upgradeDevId:" + EncryptUtils::EncryptString(upgradeDevId);
    output += ",controlDevId:" + EncryptUtils::EncryptString(controlDevId);
    return output;
}
} // namespace OHOS::UpdateEngine