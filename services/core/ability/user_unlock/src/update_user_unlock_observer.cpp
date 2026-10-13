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

#include "update_user_unlock_observer.h"

#include <string>
#include <utility>

#include "common_event_manager.h"
#include "common_event_support.h"
#include "ffrt.h"

#include "update_log.h"

namespace OHOS {
namespace UpdateService {

UserUnlockSubscriber::UserUnlockSubscriber(
    const EventFwk::CommonEventSubscribeInfo &subscribeInfo,
    OnUnlockedCallback onUnlocked)
    : EventFwk::CommonEventSubscriber(subscribeInfo),
      onUnlocked_(std::move(onUnlocked))
{}

UserUnlockSubscriber::~UserUnlockSubscriber() = default;

void UserUnlockSubscriber::OnReceiveEvent(const EventFwk::CommonEventData &data)
{
    const auto &action = data.GetWant().GetAction();
    if (action != EventFwk::CommonEventSupport::COMMON_EVENT_USER_UNLOCKED) {
        return;
    }

    ENGINE_LOGI("Received USER_UNLOCKED event, userId %{public}d", data.GetCode());
    ffrt::submit([onUnlocked = onUnlocked_]() {
        if (onUnlocked != nullptr) {
            onUnlocked();
        }
    });
}

UpdateUserUnlockObserver::UpdateUserUnlockObserver(UnlockCallback onUnlocked)
    : onUnlocked_(std::move(onUnlocked))
{}

UpdateUserUnlockObserver::~UpdateUserUnlockObserver()
{
    Stop();
}

void UpdateUserUnlockObserver::Start()
{
    ENGINE_LOGI("Start user unlock observer");
    int32_t userId = userAccountHelper_.GetActiveUserId();
    if (userAccountHelper_.IsActiveUserUnlocked(userId)) {
        ENGINE_LOGI("Active user is already unlocked, initialize update business");
        TryNotifyUnlocked();
        return;
    }
    RegisterUserUnlockListener();
    // 防止注册过程中解锁还没触发监听
    if (userAccountHelper_.IsActiveUserUnlocked(userId)) {
        ENGINE_LOGI("Active user is already unlocked");
        TryNotifyUnlocked();
    }

    ENGINE_LOGI("Active user is not unlocked yet, waiting for USER_UNLOCKED event");
}

void UpdateUserUnlockObserver::Stop()
{
    UnregisterUserUnlockListener();
}

bool UpdateUserUnlockObserver::HasUserUnlocked() const
{
    return initialized_.load();
}

void UpdateUserUnlockObserver::RegisterUserUnlockListener()
{
    std::lock_guard<std::mutex> lock(subscriberLock_);
    if (userUnlockSubscriber_ != nullptr) {
        ENGINE_LOGI("User unlock listener is already registered");
        return;
    }

    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_UNLOCKED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    const auto weakThis = weak_from_this();
    userUnlockSubscriber_ = std::make_shared<UserUnlockSubscriber>(
        subscribeInfo,
        [weakThis]() {
            const auto observer = weakThis.lock();
            if (observer == nullptr) {
                ENGINE_LOGI("User unlock callback skipped because observer is expired");
                return;
            }
            observer->TryNotifyUnlocked();
        });
    if (!EventFwk::CommonEventManager::SubscribeCommonEvent(userUnlockSubscriber_)) {
        ENGINE_LOGE("Failed to subscribe user unlock listener");
        userUnlockSubscriber_ = nullptr;
        return;
    }

    ENGINE_LOGI("Subscribe user unlock listener success");
}

void UpdateUserUnlockObserver::UnregisterUserUnlockListener()
{
    std::lock_guard<std::mutex> lock(subscriberLock_);
    if (userUnlockSubscriber_ == nullptr) {
        ENGINE_LOGI("User unlock listener is already null");
        return;
    }

    if (!EventFwk::CommonEventManager::UnSubscribeCommonEvent(userUnlockSubscriber_)) {
        ENGINE_LOGE("Failed to unsubscribe user unlock listener");
    } else {
        ENGINE_LOGI("Unsubscribe user unlock listener success");
    }
    userUnlockSubscriber_ = nullptr;
}

void UpdateUserUnlockObserver::TryNotifyUnlocked()
{
    bool expected = false;
    if (!initialized_.compare_exchange_strong(expected, true)) {
        ENGINE_LOGI("Update business is already initialized");
        UnregisterUserUnlockListener();
        return;
    }

    ENGINE_LOGI("Start update business initialization");
    if (onUnlocked_ != nullptr) {
        onUnlocked_();
    }
    // 退订涉及公共事件 IPC，放到 ffrt 线程执行，避免阻塞当前线程；捕 weak_ptr 防止对象已析构
    const auto weakThis = weak_from_this();
    ffrt::submit([weakThis]() {
        const auto observer = weakThis.lock();
        if (observer != nullptr) {
            observer->UnregisterUserUnlockListener();
        }
    });
    ENGINE_LOGI("Finish update business initialization");
}
} // namespace UpdateService
} // namespace OHOS
