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

#include "factory_reset_strategy.h"
#include "update_log.h"

namespace OHOS::UpdateService {
bool FactoryResetStrategy::ReadFromParcel(Parcel &parcel)
{
    scope = static_cast<FactoryResetScope>(parcel.ReadUint32());
    strategy = Str16ToStr8(parcel.ReadString16());
    return true;
}

bool FactoryResetStrategy::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteUint32(static_cast<uint32_t>(scope))) {
        ENGINE_LOGE("Write scope failed");
        return false;
    }
    if (!parcel.WriteString16(Str8ToStr16(strategy))) {
        ENGINE_LOGE("Write strategy failed");
        return false;
    }
    return true;
}

FactoryResetStrategy *FactoryResetStrategy::Unmarshalling(Parcel &parcel)
{
    FactoryResetStrategy *factoryResetStrategy = new (std::nothrow) FactoryResetStrategy();
    if (factoryResetStrategy == nullptr) {
        ENGINE_LOGE("Create factoryResetStrategy failed");
        return nullptr;
    }

    if (!factoryResetStrategy->ReadFromParcel(parcel)) {
        ENGINE_LOGE("Read from parcel failed");
        delete factoryResetStrategy;
        return nullptr;
    }
    return factoryResetStrategy;
}
} // namespace OHOS::UpdateService
