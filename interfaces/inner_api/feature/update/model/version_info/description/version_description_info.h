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

#ifndef VERSION_DESCRIPTION_INFO_H
#define VERSION_DESCRIPTION_INFO_H

#include <string_ex.h>
#include <vector>

#include "component_description.h"
#include "parcel.h"

namespace OHOS::UpdateEngine {
struct VersionDescriptionInfo : public Parcelable {
    std::vector<ComponentDescription> componentDescriptions;

    bool ReadFromParcel(Parcel &parcel);
    bool Marshalling(Parcel &parcel) const override;
    static VersionDescriptionInfo *Unmarshalling(Parcel &parcel);
};
} // namespace OHOS::UpdateEngine
#endif // VERSION_DESCRIPTION_INFO_H
