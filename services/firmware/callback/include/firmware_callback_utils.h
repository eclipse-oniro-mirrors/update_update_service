/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef FIRMWARE_CALLBACK_UTILS_H
#define FIRMWARE_CALLBACK_UTILS_H

#include "singleton.h"

#include "base_callback_utils.h"

namespace OHOS {
namespace UpdateService {
class FirmwareCallbackUtils final : public BaseCallbackUtils, public DelayedSingleton<FirmwareCallbackUtils> {
    DECLARE_DELAYED_SINGLETON(FirmwareCallbackUtils);

protected:
    BusinessSubType GetBusinessSubType() final;
};
} // namespace UpdateService
} // namespace OHOS
#endif // FIRMWARE_CALLBACK_UTILS_H