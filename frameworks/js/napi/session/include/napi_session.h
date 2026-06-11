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

#ifndef NAPI_SESSION_H
#define NAPI_SESSION_H

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "base_client.h"
#include "base_session.h"
#include "business_error.h"
#include "call_result.h"
#include "napi_common_define.h"
#include "napi_common_utils.h"
#include "napi_structs_base.h"

namespace OHOS::UpdateService {
class NapiSession : public BaseSession {
public:
    NapiSession(BaseClient *client, SessionParams &sessionParams, size_t argc, size_t callbackNumber);

    virtual ~NapiSession() {}

    napi_value StartWork(napi_env env, const napi_value *args, DoWorkFunction worker, void *context) override;

    BaseClient* GetNapiClient() const
    {
        return client_;
    }

    uint32_t GetType() const override
    {
        return sessionParams_.type;
    }

    uint32_t GetSessionId() const override
    {
        return sessionId;
    }

    void OnAsyncComplete(const BusinessError &businessError) final
    {
        std::lock_guard<std::mutex> lock(conditionVariableMutex_);
        businessError_ = businessError;
        asyncExecuteComplete_ = true;
        conditionVariable_.notify_all();
    }

    virtual void CompleteWork(napi_env env, napi_status status) {}
    virtual void ExecuteWork(napi_env env);
    virtual napi_value StartWork(napi_env env, size_t startIndex, const napi_value *args) = 0;

    static void CompleteWork(napi_env env, napi_status status, void *data);
    static void ExecuteWork(napi_env env, void *data);

    bool IsAsyncCompleteWork() override
    {
        return false;
    }

protected:
    napi_value CreateWorkerName(napi_env env) const;

    bool IsWorkExecuteSuccess() const
    {
        return workResult_ == INT_CALL_SUCCESS;
    }

    virtual std::string GetFunctionName()
    {
        return "";
    }

    virtual std::string GetFunctionPermissionName()
    {
        return "";
    }

    std::string GetCommonErrorMessage()
    {
        std::string msg = "";
        std::string funcName;
        std::string permissionName;
        switch (workResult_) {
            case INT_NOT_SYSTEM_APP:
                msg = "BusinessError " + std::to_string(workResult_) + NOT_SYSTEM_APP_INFO.data();
                break;
            case INT_APP_NOT_GRANTED:
                GetSessionFuncParameter(funcName, permissionName);
                msg = "BusinessError " + std::to_string(workResult_) + ": Permission denied. An attempt was made to " +
                    funcName + " forbidden by permission: " + permissionName + ".";
                break;
            case INT_MDM_DISABLE_RESET:
                msg = "BusinessError " + std::to_string(workResult_) +
                    ": This function is prohibited by enterprise management policies.";
                break;
            case INT_UN_SUPPORT:
                GetSessionFuncParameter(funcName, permissionName);
                msg = "BusinessError " + std::to_string(workResult_) + ": Capability not supported. " +
                "function " + funcName + " can not work correctly due to limited device capabilities.";
                break;
            case INT_PARAM_ERR:
                msg = "BusinessError " + std::to_string(workResult_) + ": Parameter verification failed.";
                break;
            default:
                break;
        }
        return msg;
    }

    std::string GetComponentErrorMessage()
    {
        std::string msg = "";
        switch (workResult_) {
            case INT_CALL_IPC_ERR:
                msg = "BusinessError " + std::to_string(COMPONENT_ERR + workResult_) + ": IPC error.";
                break;
            case INT_CALL_FAIL:
                msg = "BusinessError " + std::to_string(COMPONENT_ERR + workResult_) + ": Execute fail.";
                break;
            case INT_FORBIDDEN:
                msg = "BusinessError " + std::to_string(COMPONENT_ERR + workResult_) + ": Forbidden execution.";
                break;
            case INT_DEV_UPG_INFO_ERR:
                msg = "BusinessError " + std::to_string(COMPONENT_ERR + workResult_) + ": Device info error.";
                break;
            case INT_TIME_OUT:
                msg = "BusinessError " + std::to_string(COMPONENT_ERR + workResult_) + ": Execute timeout.";
                break;
            case INT_DB_ERROR:
                msg = "BusinessError " + std::to_string(COMPONENT_ERR + workResult_) + ": DB error.";
                break;
            case INT_IO_ERROR:
                msg = "BusinessError " + std::to_string(COMPONENT_ERR + workResult_) + ": IO error.";
                break;
            case INT_NET_ERROR:
                msg = "BusinessError " + std::to_string(COMPONENT_ERR + workResult_) + ": Network error.";
                break;
            default:
                break;
        }
        return msg;
    }

    void BuildWorkBusinessErr(BusinessError &businessError)
    {
        std::string message = GetComponentErrorMessage() + GetCommonErrorMessage();
        if (message.empty()) {
            message = "execute error";
        }
        businessError.Build(static_cast<CallResult>(workResult_), message);
    }

    void GetBusinessError(BusinessError &businessError, const NapiResult &result)
    {
        if (IsWorkExecuteSuccess()) {
            businessError = result.businessError;
        } else {
            BuildWorkBusinessErr(businessError);
        }
    }

protected:
    uint32_t sessionId { 0 };
    BaseClient *client_ = nullptr;
    BusinessError businessError_ {};
    SessionParams sessionParams_ {};
    int32_t workResult_ = INT_CALL_SUCCESS;
    size_t totalArgc_ = 0;
    size_t callbackNumber_ = 0;
    void* context_ {};
    DoWorkFunction doWorker_ {};
    std::condition_variable conditionVariable_;
    std::mutex conditionVariableMutex_;
    bool asyncExecuteComplete_ = false;

private:
    void GetSessionFuncParameter(std::string &funcName, std::string &permissionName);
};
} // namespace OHOS::UpdateService
#endif // NAPI_SESSION_H