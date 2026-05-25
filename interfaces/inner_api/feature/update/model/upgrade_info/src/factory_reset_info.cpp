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

#include "factory_reset_info.h"
#include "update_log.h"

namespace OHOS::UpdateService {
bool FactoryResetInfo::ReadFromParcel(Parcel &parcel)
{
    duration = parcel.ReadInt32();
    return true;
}

bool FactoryResetInfo::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(duration)) {
        ENGINE_LOGE("Write duration failed");
        return false;
    }
    return true;
}

FactoryResetInfo *FactoryResetInfo::Unmarshalling(Parcel &parcel)
{
    FactoryResetInfo *factoryResetInfo = new (std::nothrow) FactoryResetInfo();
    if (factoryResetInfo == nullptr) {
        ENGINE_LOGE("Create factoryResetInfo failed");
        return nullptr;
    }

    if (!factoryResetInfo->ReadFromParcel(parcel)) {
        ENGINE_LOGE("Read from parcel failed");
        delete factoryResetInfo;
        return nullptr;
    }
    return factoryResetInfo;
}
} // namespace OHOS::UpdateService
