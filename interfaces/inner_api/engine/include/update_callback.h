/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef UPDATE_CALLBACK_H
#define UPDATE_CALLBACK_H

#include "update_callback_info.h"
#include "update_callback_stub.h"

namespace OHOS::UpdateService {
class UpdateCallback : public UpdateCallbackStub {
public:
    explicit UpdateCallback() = default;

    explicit UpdateCallback(const UpdateCallbackInfo &updateCallback);

    ~UpdateCallback() override = default;

    ErrCode OnEvent(const EventInfo &eventInfo) override;

private:
    UpdateCallbackInfo updateCallback_{};
};
} // namespace OHOS::UpdateService
#endif // UPDATE_CALLBACK_H