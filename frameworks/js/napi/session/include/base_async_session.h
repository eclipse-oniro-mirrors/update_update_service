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

#ifndef BASE_ASYNC_SESSION_H
#define BASE_ASYNC_SESSION_H

#include <mutex>
#include "napi/native_api.h"
#include "node_api.h"

#include "napi_session.h"

namespace OHOS::UpdateService {
constexpr int32_t RESULT_ARGC = 2;

template <typename RESULT> class BaseAsyncSession : public NapiSession {
public:
    BaseAsyncSession(BaseClient *client, SessionParams &sessionParams, size_t argc, size_t callbackNumber = 1)
        : NapiSession(client, sessionParams, argc, callbackNumber)
    {
        callbackRef_.resize(callbackNumber);
    }

    ~BaseAsyncSession() override
    {
        callbackRef_.clear();
    }

    void  ReleaseNapiReference(napi_env env)
    {
        std::lock_guard<std::mutex> lock(callbackRefMutex_);
        for (size_t i = 0; i < callbackRef_.size(); i++) {
            if (callbackRef_[i] != nullptr) {
                napi_status status = napi_delete_reference(env, callbackRef_[i]);
                if (status != napi_ok) {
                    ENGINE_LOGE("callbackRef %{public}zu release failed", i);
                }
                callbackRef_[i] = nullptr;
            }
        }
    }

    napi_value StartWork(napi_env env, size_t startIndex, const napi_value *args) override
    {
        ENGINE_LOGI("BaseAsyncSession::StartWork startIndex: %{public}zu totalArgc_ %{public}zu "
            "callbackNumber_: %{public}zu",
            startIndex, totalArgc_, callbackNumber_);
        PARAM_CHECK_NAPI_CALL(env, args != nullptr && totalArgc_ >= startIndex, return nullptr, "Invalid para");
        napi_value workName = CreateWorkerName(env);
        PARAM_CHECK_NAPI_CALL(env, workName != nullptr, return nullptr, "Failed to worker name");

        // Check whether a callback exists. Only one callback is allowed.
        for (size_t i = 0; (i < (totalArgc_ - startIndex)) && (i < callbackNumber_); i++) {
            ENGINE_LOGI("CreateReference index:%u", static_cast<unsigned int>(i + startIndex));
            ClientStatus ret = NapiCommonUtils::IsTypeOf(env, args[i + startIndex], napi_function);
            std::vector<std::pair<std::string, std::string>> paramInfos;
            paramInfos.emplace_back("callback", "napi_function");
            PARAM_CHECK_NAPI_CALL(env, ret == ClientStatus::CLIENT_SUCCESS,
                NapiCommonUtils::NapiThrowParamError(env, paramInfos);
                return nullptr, "invalid type");
            std::lock_guard<std::mutex> lock(callbackRefMutex_);
            ret = NapiCommonUtils::CreateReference(env, args[i + startIndex], 1, callbackRef_[i]);
            PARAM_CHECK_NAPI_CALL(env, ret == ClientStatus::CLIENT_SUCCESS, return nullptr,
                "Failed to create reference");
        }

        napi_status status = napi_ok;
        // Create an asynchronous call.
        status = napi_create_async_work(env, nullptr, workName, NapiSession::ExecuteWork, NapiSession::CompleteWork,
            this, &(worker_));

        PARAM_CHECK_NAPI_CALL(env, status == napi_ok, ReleaseNapiReference(env);return nullptr,
            "Failed to create worker");

        // Put the thread in the task execution queue.
        status = napi_queue_async_work_with_qos(env, worker_, napi_qos_default);
        PARAM_CHECK_NAPI_CALL(env, status == napi_ok, ReleaseNapiReference(env);return nullptr,
            "Failed to queue worker");
        napi_value result;
        napi_create_int32(env, 0, &result);
        return result;
    }

    void NotifyJS(napi_env env, napi_value thisVar, const RESULT &result)
    {
        ENGINE_LOGI("BaseAsyncSession::NotifyJS callbackNumber_: %{public}d", static_cast<int32_t>(callbackNumber_));
        napi_handle_scope scope;
        napi_status status = napi_open_handle_scope(env, &scope);
        PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return, "Error open_handle_scope");
        napi_value callback;
        napi_value undefined;
        napi_value callResult;
        napi_get_undefined(env, &undefined);
        napi_value retArgs[RESULT_ARGC] = { 0 };
        napi_status retStatus;

        BusinessError businessError;
        GetBusinessError(businessError, result);
        uint32_t ret;
        if (businessError.errorNum != CallResult::SUCCESS) {
            ENGINE_LOGI("BaseAsyncSession::NotifyJS error exist");
            ret = static_cast<uint32_t>(NapiCommonUtils::BuildBusinessError(env, retArgs[0], businessError));
        } else {
            ret = static_cast<uint32_t>(result.buildJSObject(env, retArgs[1], result));
        }
        PARAM_CHECK_NAPI_CALL(env, ret == napi_ok, napi_close_handle_scope(env, scope);return,
            "Failed to build json");
        {
            std::lock_guard<std::mutex> lock(callbackRefMutex_);
            retStatus = napi_get_reference_value(env, callbackRef_[0], &callback);
        }
        PARAM_CHECK_NAPI_CALL(env, retStatus == napi_ok, napi_close_handle_scope(env, scope);return,
            "Failed to get reference");
        const int callBackNumber = 2;
        retStatus = napi_call_function(env, undefined, callback, callBackNumber, retArgs, &callResult);
        PARAM_CHECK_NAPI_CALL(env, retStatus == napi_ok, napi_close_handle_scope(env, scope);return,
            "Failed to call function");
        // Release resources.
        ReleaseNapiReference(env);
        napi_delete_async_work(env, worker_);
        worker_ = nullptr;
        napi_close_handle_scope(env, scope);
    }

    virtual void CompleteWork(napi_env env, napi_status status) override{};

protected:
    napi_async_work worker_ = nullptr;
    std::vector<napi_ref> callbackRef_ = { 0 };
    std::mutex callbackRefMutex_;
};
} // namespace OHOS::UpdateService
#endif // BASE_ASYNC_SESSION_H