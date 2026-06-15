/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ani_base_updater.h"

#include "taihe/runtime.hpp"

#include "ani_common_conveter.h"
#include "update_log.h"

namespace OHOS::UpdateService {
AniBaseUpdater::~AniBaseUpdater()
{
    OffAll();
}

void AniBaseUpdater::On(taihe::callback_view<void(const ohos::update::EventInfo &)> callback)
{
    std::unique_lock lock(mutex_);
    taihe::callback<void(const ohos::update::EventInfo &)> taiheCallback(callback);
    for (const auto &callbackInfo : callbacks_) {
        if (taiheCallback == callbackInfo) {
            ENGINE_LOGI("AniBaseUpdater::on ignored, callback is same");
            return;
        }
    }
    if (callbacks_.empty()) {
        RegisterCallback();
    }
    callbacks_.push_back(taiheCallback);
}

void AniBaseUpdater::Off([[maybe_unused]] const taihe::callback<void(const ohos::update::EventInfo &)> &callback)
{
    std::unique_lock lock(mutex_);
    callbacks_.erase(
        std::remove_if(callbacks_.begin(), callbacks_.end(),
            [&](const auto &cb) { return cb == callback; }),
            callbacks_.end());
    if (callbacks_.empty()) {
        UnRegisterCallback();
    }
}

void AniBaseUpdater::OffAll()
{
    std::unique_lock lock(mutex_);
    callbacks_.clear();
    UnRegisterCallback();
}

std::string AniBaseUpdater::GetPermissionName()
{
    return "ohos.permission.UPDATE_SYSTEM";
}

bool AniBaseUpdater::SetError(int32_t ret, const std::string &funcName, const BusinessError &error)
{
    if (ret != 0) {
        const BusinessError errorInfo = GetIpcBusinessError(funcName, ret);
        taihe::set_business_error(ConvertToErrorCode(errorInfo.errorNum), errorInfo.message);
        return true;
    }
    if (!error.IsSuccess()) {
        taihe::set_business_error(ConvertToErrorCode(error.errorNum), error.message);
        return true;
    }
    return false;
}

void AniBaseUpdater::CallbackEventInfo(const EventInfo &eventInfo)
{
    std::unique_lock lock(mutex_);
    for ([[maybe_unused]] const auto &callback : callbacks_) {
        callback(AniCommonConverter::Converter(eventInfo));
    }
}

bool AniBaseUpdater::IsCommonError(CallResult callResult)
{
    return callResult == CallResult::UN_SUPPORT || callResult == CallResult::NOT_SYSTEM_APP ||
        callResult == CallResult::APP_NOT_GRANTED || callResult == CallResult::PARAM_ERR;
}

int32_t AniBaseUpdater::ConvertToErrorCode(CallResult callResult)
{
    if (IsCommonError(callResult) || callResult == CallResult::SUCCESS) {
        return CAST_INT(callResult);
    }
    constexpr int32_t componentErr = 11500000;
    return componentErr + CAST_INT(callResult);
}

void AniBaseUpdater::GetIpcErrorMessage(const std::string &funcName, int32_t ipcRequestCode,
    const std::string &callResultStr, std::string &msg)
{
    switch (ipcRequestCode) {
        case INT_NOT_SYSTEM_APP:
            msg = "BusinessError " + callResultStr + NOT_SYSTEM_APP_INFO.data();
            break;
        case INT_APP_NOT_GRANTED:
            msg = "BusinessError " + callResultStr + ": Permission denied. An attempt was made to " + funcName +
                " forbidden by permission: " + GetPermissionName() + ".";
            break;
        case INT_CALL_IPC_ERR:
            msg = "BusinessError " + callResultStr + ": IPC error.";
            break;
        case INT_UN_SUPPORT:
            msg = "BusinessError " + callResultStr + ": Capability not supported. " + "function " + funcName +
                " can not work correctly due to limited device capabilities.";
            break;
        case INT_PARAM_ERR:
            msg = "param error";
            break;
        case INT_MDM_DISABLE_RESET:
            msg = "BusinessError " + callResultStr +
                ": This function is prohibited by enterprise management policies.";
            break;
        case INT_CALL_FAIL:
            msg = "BusinessError " + callResultStr + ": Execute fail.";
            break;
        case INT_FORBIDDEN:
            msg = "BusinessError " + callResultStr + ": Forbidden execution.";
            break;
        case INT_DEV_UPG_INFO_ERR:
            msg = "BusinessError " + callResultStr + ": Device info error.";
            break;
        case INT_TIME_OUT:
            msg = "BusinessError " + callResultStr + ": Execute timeout.";
            break;
        case INT_DB_ERROR:
            msg = "BusinessError " + callResultStr + ": DB error.";
            break;
        case INT_IO_ERROR:
            msg = "BusinessError " + callResultStr + ": IO error.";
            break;
        case INT_NET_ERROR:
            msg = "BusinessError " + callResultStr + ": Network error.";
            break;
        default:
            break;
    }
}

BusinessError AniBaseUpdater::GetIpcBusinessError(const std::string &funcName, int32_t ipcRequestCode)
{
    BusinessError businessError;
    std::string errorMessage = "execute error";
    const auto ipcCallResult = static_cast<CallResult>(ipcRequestCode);
    const std::string ipcCallResultStr = std::to_string(ConvertToErrorCode(ipcCallResult));

    GetIpcErrorMessage(funcName, ipcRequestCode, ipcCallResultStr, errorMessage);
    businessError.Build(static_cast<CallResult>(ipcRequestCode), errorMessage);
    return businessError;
}

}
