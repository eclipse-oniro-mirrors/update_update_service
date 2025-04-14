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

#ifndef UPDATE_SERVICE_COMPONENT_DESCRIPTION_H
#define UPDATE_SERVICE_COMPONENT_DESCRIPTION_H

#include <string>

#include "description_info.h"
#include "parcel.h"

namespace OHOS::UpdateService {
struct ComponentDescription : public Parcelable {
    std::string componentId;
    DescriptionInfo descriptionInfo;
    DescriptionInfo notifyDescriptionInfo;

    bool ReadFromParcel(Parcel &parcel);
    bool Marshalling(Parcel &parcel) const override;
    static ComponentDescription *Unmarshalling(Parcel &parcel);
};
} // namespace OHOS::UpdateService
#endif // UPDATE_SERVICE_COMPONENT_DESCRIPTION_H
