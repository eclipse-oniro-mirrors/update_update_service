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

#include "business_error.h"
#include "parcel_common.h"

namespace OHOS::UpdateService {
bool BusinessError::ReadFromParcel(Parcel &parcel)
{
    message = Str16ToStr8(parcel.ReadString16());
    errorNum = static_cast<CallResult>(parcel.ReadInt32());

    int32_t errorMessageSize = parcel.ReadInt32();
    if (errorMessageSize == 0) {
        ENGINE_LOGE("error message size is zero");
        return true;
    }
    if ((errorMessageSize < MIN_VECTOR_SIZE) || (errorMessageSize > MAX_VECTOR_SIZE)) {
        ENGINE_LOGE("Invalid error message size =%{public}d", errorMessageSize);
        return false;
    }

    for (size_t i = 0; i < static_cast<size_t>(errorMessageSize); i++) {
        sptr<ErrorMessage> unmarshallingErrorMsg = ErrorMessage().Unmarshalling(parcel);
        if (unmarshallingErrorMsg != nullptr) {
            data.emplace_back(*unmarshallingErrorMsg);
        } else {
            ENGINE_LOGE("unmarshallingErrorMsg is null");
            return false;
        }
    }

    return true;
}

bool BusinessError::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString16(Str8ToStr16(message))) {
        ENGINE_LOGE("Write message failed");
        return false;
    }

    if (!parcel.WriteInt32(static_cast<int32_t>(errorNum))) {
        ENGINE_LOGE("Write errorNum failed");
        return false;
    }

    parcel.WriteInt32(static_cast<int32_t>(data.size()));
    for (size_t i = 0; i < data.size(); i++) {
        data[i].Marshalling(parcel);
    }

    return true;
}

BusinessError *BusinessError::Unmarshalling(Parcel &parcel)
{
    BusinessError *businessError = new (std::nothrow) BusinessError();
    if (businessError == nullptr) {
        ENGINE_LOGE("Create businessError failed");
        return nullptr;
    }

    if (!businessError->ReadFromParcel(parcel)) {
        ENGINE_LOGE("Read from parcel failed");
        delete businessError;
        return nullptr;
    }
    return businessError;
}
} // namespace OHOS::UpdateService
