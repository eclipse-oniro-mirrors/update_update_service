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

#ifndef COMMON_DEATH_RECIPIENT_H
#define COMMON_DEATH_RECIPIENT_H

#include <utility>

#include "iremote_object.h"

namespace OHOS::UpdateEngine {
class CommonDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    using ResetServiceFunc = std::function<void(const wptr<IRemoteObject> &object)>;

    explicit CommonDeathRecipient(ResetServiceFunc resetFunc) : resetServiceFunc_(std::move(resetFunc)) {}

    ~CommonDeathRecipient() override = default;

    void OnRemoteDied(const wptr<IRemoteObject> &object) override
    {
        if (this->resetServiceFunc_) {
            this->resetServiceFunc_(object);
        }
    }

private:
    ResetServiceFunc resetServiceFunc_;
};
} // namespace OHOS::UpdateEngine
#endif // COMMON_DEATH_RECIPIENT_H
