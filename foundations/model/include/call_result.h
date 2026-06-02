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

#ifndef UPDATE_SERVICE_CALL_RESULT_H
#define UPDATE_SERVICE_CALL_RESULT_H

#include "update_define.h"

namespace OHOS::UpdateService {
constexpr int CALL_RESULT_OFFSET = 2000;

enum class CallResult {
    // 通用错误码
    APP_NOT_GRANTED = 201,
    NOT_SYSTEM_APP = 202,
    MDM_DISABLE_RESET = 203,
    PARAM_ERR = 401,
    UN_SUPPORT = 801,

    // 模块内错误码
    SUCCESS = 0,
    FAIL = 100,
    DEV_UPG_INFO_ERR = 102,
    FORBIDDEN = 103,
    IPC_ERR = 104,
    TIME_OUT = 402,
    DB_ERROR = 501,
    IO_ERROR = 502,
    NET_ERROR = 503,
    CANCEL_ERROR = 504
};

constexpr int32_t INT_CALL_SUCCESS = CAST_INT(CallResult::SUCCESS);
constexpr int32_t INT_CALL_FAIL = CAST_INT(CallResult::FAIL);
constexpr int32_t INT_UN_SUPPORT = CAST_INT(CallResult::UN_SUPPORT);
constexpr int32_t INT_FORBIDDEN = CAST_INT(CallResult::FORBIDDEN);
constexpr int32_t INT_CALL_IPC_ERR = CAST_INT(CallResult::IPC_ERR);
constexpr int32_t INT_APP_NOT_GRANTED = CAST_INT(CallResult::APP_NOT_GRANTED);
constexpr int32_t INT_NOT_SYSTEM_APP = CAST_INT(CallResult::NOT_SYSTEM_APP);
constexpr int32_t INT_PARAM_ERR = CAST_INT(CallResult::PARAM_ERR);
constexpr int32_t INT_DEV_UPG_INFO_ERR = CAST_INT(CallResult::DEV_UPG_INFO_ERR);
constexpr int32_t INT_TIME_OUT = CAST_INT(CallResult::TIME_OUT);
constexpr int32_t INT_DB_ERROR = CAST_INT(CallResult::DB_ERROR);
constexpr int32_t INT_IO_ERROR = CAST_INT(CallResult::IO_ERROR);
constexpr int32_t INT_NET_ERROR = CAST_INT(CallResult::NET_ERROR);
constexpr int32_t INT_MDM_DISABLE_RESET = CAST_INT(CallResult::MDM_DISABLE_RESET);
constexpr std::string_view NOT_SYSTEM_APP_INFO =
    ": Permission verification failed. A non-system application calls a system API.";
} // namespace OHOS::UpdateService
#endif // UPDATE_SERVICE_CALL_RESULT_H
