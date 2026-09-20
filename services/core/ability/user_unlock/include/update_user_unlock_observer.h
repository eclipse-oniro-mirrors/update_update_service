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

#ifndef UPDATE_USER_UNLOCK_OBSERVER_H
#define UPDATE_USER_UNLOCK_OBSERVER_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "user_account_helper.h"

namespace OHOS {
namespace UpdateService {
class UserUnlockListener;

class UpdateUserUnlockObserver : public std::enable_shared_from_this<UpdateUserUnlockObserver> {
public:
    using UnlockCallback = std::function<void()>;

    explicit UpdateUserUnlockObserver(UnlockCallback onUnlocked);
    ~UpdateUserUnlockObserver();

    void Start();
    void Stop();
    // 检查是否解锁
    bool HasUserUnlocked() const;

private:
    void RegisterUserUnlockListener();
    void UnregisterUserUnlockListener();
    void TryNotifyUnlocked();

    UnlockCallback onUnlocked_;
    UserAccountHelper userAccountHelper_;
    std::atomic<bool> initialized_ = false;
    std::shared_ptr<UserUnlockListener> listener_ = nullptr;
};
} // namespace UpdateService
} // namespace OHOS
#endif // UPDATE_USER_UNLOCK_OBSERVER_H
