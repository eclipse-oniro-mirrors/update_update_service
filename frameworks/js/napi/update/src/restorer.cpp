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

#include "restorer.h"

#include "client_helper.h"
#include "update_service_kits.h"

namespace OHOS::UpdateService {
napi_value Restorer::Napi::FactoryReset(napi_env env, napi_callback_info info)
{
    ENGINE_LOGI("Restorer::Napi::FactoryReset");
    Restorer* restorer = UnwrapJsObject<Restorer>(env, info);
    PARAM_CHECK_NAPI_CALL(env, restorer != nullptr, return nullptr, "Error get restorer");
    return restorer->FactoryReset(env, info);
}

napi_value Restorer::Napi::ForceFactoryReset(napi_env env, napi_callback_info info)
{
    ENGINE_LOGI("Restorer::Napi::ForceFactoryReset");
    Restorer* restorer = UnwrapJsObject<Restorer>(env, info);
    PARAM_CHECK_NAPI_CALL(env, restorer != nullptr, return nullptr, "Error get restorer");
    return restorer->ForceFactoryReset(env, info);
}

napi_value Restorer::Napi::DeepFactoryReset(napi_env env, napi_callback_info info)
{
    ENGINE_LOGI("Restorer::Napi::DeepFactoryReset");
    Restorer* restorer = UnwrapJsObject<Restorer>(env, info);
    PARAM_CHECK_NAPI_CALL(env, restorer != nullptr, return nullptr, "Error get restorer");
    return restorer->DeepFactoryReset(env, info);
}

napi_value Restorer::Napi::GetDeepFactoryResetInfo(napi_env env, napi_callback_info info)
{
    ENGINE_LOGI("Restorer::Napi::GetDeepFactoryResetInfo");
    Restorer* restorer = UnwrapJsObject<Restorer>(env, info);
    PARAM_CHECK_NAPI_CALL(env, restorer != nullptr, return nullptr, "Error get restorer");
    return restorer->GetDeepFactoryResetInfo(env, info);
}

Restorer::Restorer(napi_env env, napi_value thisVar)
{
    napi_ref thisReference = nullptr;
    constexpr int32_t refCount = 1; // 新引用的初始引用计数
    napi_create_reference(env, thisVar, refCount, &thisReference);
    sessionsMgr_ = std::make_shared<SessionManager>(env, thisReference);
    ENGINE_LOGI("Restorer::Restorer");
}

napi_value Restorer::FactoryReset(napi_env env, napi_callback_info info)
{
    SessionParams sessionParams(SessionType::SESSION_FACTORY_RESET, CALLBACK_POSITION_ONE, true);
    napi_value retValue = StartSession(env, info, sessionParams,
        [](void *context) -> int {
            BusinessError *businessError = reinterpret_cast<BusinessError *>(context);
            return UpdateServiceKits::GetInstance().FactoryReset(*businessError);
        });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to FactoryReset.");
    return retValue;
}

napi_value Restorer::ForceFactoryReset(napi_env env, napi_callback_info info)
{
    SessionParams sessionParams(SessionType::SESSION_FORCE_FACTORY_RESET, CALLBACK_POSITION_ONE, true);
    napi_value retValue = StartSession(env, info, sessionParams,
        [](void *context) -> int {
            BusinessError *businessError = reinterpret_cast<BusinessError *>(context);
            return UpdateServiceKits::GetInstance().ForceFactoryReset(*businessError);
        });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to FactoryReset.");
    return retValue;
}

int32_t Restorer::ParseParamResetStrategy(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = { 0 };
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return -1, "Error to get cb info.");
    ClientStatus ret = ClientHelper::GetFactoryResetStrategyFromArg(env, args[0], factoryResetStrategy_);
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS, return -1, "Failed to get factoryResetStrategy param");
    return 0;
}

napi_value Restorer::DeepFactoryReset(napi_env env, napi_callback_info info)
{
    int32_t ret = ParseParamResetStrategy(env, info);
    PARAM_CHECK(ret == 0, return nullptr, "Failed to parse param factoryResetStrategy.");
    SessionParams sessionParams(SessionType::SESSION_DEEP_FACTORY_RESET, CALLBACK_POSITION_TWO, true);
    napi_value retValue = StartSession(env, info, sessionParams,
        [=](void *context) -> int {
            BusinessError *businessError = reinterpret_cast<BusinessError *>(context);
            return UpdateServiceKits::GetInstance().DeepFactoryReset(factoryResetStrategy_, *businessError);
        });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to DeepFactoryReset.");
    return retValue;
}

napi_value Restorer::GetDeepFactoryResetInfo(napi_env env, napi_callback_info info)
{
    int32_t ret = ParseParamResetStrategy(env, info);
    PARAM_CHECK(ret == 0, return nullptr, "Failed to parse param factoryResetStrategy.");
    SessionParams sessionParams(SessionType::SESSION_GET_DEEP_FACTORY_RESET_INFO, CALLBACK_POSITION_TWO, true);
    napi_value retValue = StartSession(env, info, sessionParams,
        [=](void *context) -> int {
            BusinessError *businessError = reinterpret_cast<BusinessError *>(context);
            return UpdateServiceKits::GetInstance().GetDeepFactoryResetInfo(factoryResetStrategy_, factoryResetInfo_,
                *businessError);
        });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to GetDeepFactoryResetInfo.");
    return retValue;
}

void Restorer::GetUpdateResult(uint32_t type, UpdateResult &result)
{
    ENGINE_LOGI("GetUpdateResult type %{public}d", type);
    result.type = type;
    switch (type) {
        case SessionType::SESSION_GET_DEEP_FACTORY_RESET_INFO:
            result.result.factoryResetInfo = &factoryResetInfo_;
            result.buildJSObject = ClientHelper::BuildFactoryResetInfo;
            break;
        default:
            result.buildJSObject = ClientHelper::BuildUndefinedStatus;
            break;
    }
}
} // namespace OHOS::UpdateService