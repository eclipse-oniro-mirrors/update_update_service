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

#ifndef UPDATE_SERVICE_VERSION_COMPONENT_H
#define UPDATE_SERVICE_VERSION_COMPONENT_H

#include <string>
#include <cstdint>

#include "base_json_struct.h"
#include "component_type.h"
#include "description_info.h"
#include "effective_mode.h"
#include "update_define.h"
#include "upgrade_action.h"
#include "ota_mode.h"

#include "parcel.h"

namespace OHOS::UpdateService {
struct VersionComponent : public BaseJsonStruct, public Parcelable {
    std::string componentId;
    int32_t componentType = CAST_INT(ComponentType::INVALID);
    std::string upgradeAction;
    std::string displayVersion;
    std::string innerVersion;
    size_t size = 0;
    size_t effectiveMode = static_cast<size_t>(EffectiveMode::COLD);
    size_t otaMode = static_cast<size_t>(OtaMode::REGULAR_OTA);
    DescriptionInfo descriptionInfo;
    std::string componentExtra;

    JsonBuilder GetJsonBuilder() final;

    bool ReadFromParcel(Parcel &parcel);
    bool Marshalling(Parcel &parcel) const override;
    static VersionComponent *Unmarshalling(Parcel &parcel);
};
} // namespace OHOS::UpdateService
#endif // UPDATE_SERVICE_VERSION_COMPONENT_H
