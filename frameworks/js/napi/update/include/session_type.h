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

#ifndef UPDATE_SESSION_TYPE_H
#define UPDATE_SESSION_TYPE_H

#include <cstdint>

namespace OHOS::UpdateService::SessionType {
constexpr uint32_t SESSION_CHECK_VERSION = 0;
constexpr uint32_t SESSION_DOWNLOAD = 1;
constexpr uint32_t SESSION_PAUSE_DOWNLOAD = 2;
constexpr uint32_t SESSION_RESUME_DOWNLOAD = 3;
constexpr uint32_t SESSION_UPGRADE = 4;
constexpr uint32_t SESSION_SET_POLICY = 5;
constexpr uint32_t SESSION_GET_POLICY = 6;
constexpr uint32_t SESSION_CLEAR_ERROR = 7;
constexpr uint32_t SESSION_TERMINATE_UPGRADE = 8;
constexpr uint32_t SESSION_GET_NEW_VERSION = 9;
constexpr uint32_t SESSION_GET_NEW_VERSION_DESCRIPTION = 10;
constexpr uint32_t SESSION_SUBSCRIBE = 11;
constexpr uint32_t SESSION_UNSUBSCRIBE = 12;
constexpr uint32_t SESSION_GET_UPDATER = 13;
constexpr uint32_t SESSION_APPLY_NEW_VERSION = 14;
constexpr uint32_t SESSION_FACTORY_RESET = 15;
constexpr uint32_t SESSION_VERIFY_PACKAGE = 16;
constexpr uint32_t SESSION_CANCEL_UPGRADE = 17;
constexpr uint32_t SESSION_GET_CUR_VERSION = 18;
constexpr uint32_t SESSION_GET_CUR_VERSION_DESCRIPTION = 19;
constexpr uint32_t SESSION_GET_TASK_INFO = 20;
constexpr uint32_t SESSION_REPLY_PARAM_ERROR = 21;
constexpr uint32_t SESSION_FORCE_FACTORY_RESET = 22;
constexpr uint32_t SESSION_DEEP_FACTORY_RESET = 23;
constexpr uint32_t SESSION_GET_DEEP_FACTORY_RESET_INFO = 24;
constexpr uint32_t SESSION_MAX = UINT32_MAX;
} // namespace OHOS::UpdateService::SessionType
#endif // UPDATE_SESSION_TYPE_H
