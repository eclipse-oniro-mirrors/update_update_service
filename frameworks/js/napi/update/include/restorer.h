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

#ifndef UPDATE_CLIENT_RESTORER_H
#define UPDATE_CLIENT_RESTORER_H

#include "js_native_api_types.h"
#include "factory_reset_info.h"
#include "factory_reset_strategy.h"

#include "iupdater.h"

namespace OHOS::UpdateService {
class Restorer : public IUpdater {
public:
    DECLARE_CLASS_NAME(Restorer);
    class Napi {
    public:
        static constexpr const char *FUNCTION_FACTORY_RESET = "factoryReset";

        static napi_value FactoryReset(napi_env env, napi_callback_info info);

        static constexpr const char *FUNCTION_FORCE_FACTORY_RESET = "forceFactoryReset";

        static napi_value ForceFactoryReset(napi_env env, napi_callback_info info);

        static constexpr const char *FUNCTION_DEEP_FACTORY_RESET = "deepFactoryReset";

        static napi_value DeepFactoryReset(napi_env env, napi_callback_info info);

        static constexpr const char *FUNCTION_GET_DEEP_FACTORY_RESET_INFO = "getDeepFactoryResetInfo";

        static napi_value GetDeepFactoryResetInfo(napi_env env, napi_callback_info info);
    };

    Restorer(napi_env env, napi_value thisVar);
    napi_value FactoryReset(napi_env env, napi_callback_info info);
    napi_value ForceFactoryReset(napi_env env, napi_callback_info info);
    napi_value DeepFactoryReset(napi_env env, napi_callback_info info);
    napi_value GetDeepFactoryResetInfo(napi_env env, napi_callback_info info);
    void GetUpdateResult(uint32_t type, UpdateResult &result);
private:
    int32_t ParseParamResetStrategy(napi_env env, napi_callback_info info);
    FactoryResetStrategy factoryResetStrategy_ {};
    FactoryResetInfo factoryResetInfo_ {};
};
} // namespace OHOS::UpdateService
#endif // UPDATE_CLIENT_RESTORER_H