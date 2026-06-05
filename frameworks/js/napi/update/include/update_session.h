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

#ifndef UPDATE_SESSION_H
#define UPDATE_SESSION_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "base_async_session.h"
#include "base_promise_session.h"
#include "iupdater.h"
#include "napi_session.h"

namespace OHOS::UpdateService {
class BaseUpdateSession : public BaseAsyncSession<UpdateResult> {
public:
    BaseUpdateSession(BaseClient *client, SessionParams &sessionParams, size_t argc, size_t callbackNumber)
        : BaseAsyncSession<UpdateResult>(client, sessionParams, argc, callbackNumber) {}

    ~BaseUpdateSession() override = default;

    void GetUpdateResult(UpdateResult &result)
    {
        result.businessError = businessError_;
        IUpdater *migrateClient = static_cast<IUpdater *>(client_);
        migrateClient->GetUpdateResult(sessionParams_.type, result);
    }

    std::string GetFunctionName() override;

    std::string GetFunctionPermissionName() override
    {
        if (sessionParams_.type == SessionType::SESSION_FACTORY_RESET) {
            return "ohos.permission.FACTORY_RESET";
        } else {
            return "ohos.permission.UPDATE_SYSTEM";
        }
    }

    bool IsAsyncCompleteWork() override
    {
        return sessionParams_.type == SessionType::SESSION_CHECK_VERSION;
    }

    virtual void CompleteWork(napi_env env, napi_status status) override
    {
        ENGINE_LOGI("BaseUpdateSession::CompleteWork callbackNumber_: %d", static_cast<int32_t>(callbackNumber_));
    }
};


class BaseMigratePromiseSession : public BasePromiseSession<UpdateResult> {
public:
    BaseMigratePromiseSession(BaseClient *client, SessionParams &sessionParams, size_t argc,
        size_t callbackNumber = 0)
        : BasePromiseSession<UpdateResult>(client, sessionParams, argc, callbackNumber) {}

    void GetUpdateResult(UpdateResult &result)
    {
        result.businessError = businessError_;
        IUpdater *migrateClient = static_cast<IUpdater *>(client_);
        if (migrateClient == nullptr) {
            ENGINE_LOGE("migrateClient is nullptr");
            return;
        }
        migrateClient->GetUpdateResult(sessionParams_.type, result);
    }

    std::string GetFunctionName() override;

    std::string GetFunctionPermissionName() override
    {
        if (sessionParams_.type == SessionType::SESSION_FACTORY_RESET ||
            sessionParams_.type == SessionType::SESSION_DEEP_FACTORY_RESET ||
            sessionParams_.type == SessionType::SESSION_GET_DEEP_FACTORY_RESET_INFO) {
            return "ohos.permission.FACTORY_RESET";
        } else {
            return "ohos.permission.UPDATE_SYSTEM";
        }
    }

    bool IsAsyncCompleteWork() override
    {
        return sessionParams_.type == SessionType::SESSION_CHECK_VERSION;
    }

    void CompleteWork(napi_env env, napi_status status) override {}
};

class UpdateAsyncession : public BaseUpdateSession {
public:
    UpdateAsyncession(IUpdater *client, SessionParams &sessionParams, size_t argc, size_t callbackNumber = 1)
        : BaseUpdateSession(client, sessionParams, argc, callbackNumber) {}

    ~UpdateAsyncession() override = default;

    void CompleteWork(napi_env env, napi_status status) override;
};

class UpdateAsyncessionNoCallback : public UpdateAsyncession {
public:
    UpdateAsyncessionNoCallback(IUpdater *client, SessionParams &sessionParams, size_t argc, size_t callbackNumber = 0)
        : UpdateAsyncession(client, sessionParams, argc, callbackNumber) {}

    ~UpdateAsyncessionNoCallback() override = default;
};

class UpdatePromiseSession : public BaseMigratePromiseSession {
public:
    UpdatePromiseSession(IUpdater *client, SessionParams &sessionParams, size_t argc, size_t callbackNumber = 0)
        : BaseMigratePromiseSession(client, sessionParams, argc, callbackNumber) {}

    ~UpdatePromiseSession() override = default;

    void CompleteWork(napi_env env, napi_status status) override;
    void GetUpdateResult(UpdateResult &result)
    {
        result.businessError = businessError_;
        IUpdater *restorerClient = static_cast<IUpdater *>(client_);
        restorerClient->GetUpdateResult(sessionParams_.type, result);
    }
};

class UpdateListener : public NapiSession {
public:
    UpdateListener(BaseClient *client, SessionParams &sessionParams, size_t argc, bool isOnce,
        size_t callbackNumber = 1)
        : NapiSession(client, sessionParams, argc, callbackNumber), isOnce_(isOnce) {}

    ~UpdateListener() override = default;

    napi_value StartWork(napi_env env, size_t startIndex, const napi_value *args) override;

    void NotifyJS(napi_env env, napi_value thisVar, const UpdateResult &result);

    void NotifyJS(napi_env env, napi_value thisVar, const EventInfo &eventInfo);

    bool IsOnce() const
    {
        return isOnce_;
    }

    std::string GetEventType() const
    {
        return eventType_;
    }

    bool CheckEqual(napi_env env, napi_value handler, const std::string &type);

    bool IsSubscribeEvent(const EventClassifyInfo &eventClassifyInfo) const
    {
        return eventClassifyInfo_.eventClassify == eventClassifyInfo.eventClassify;
    }

    bool IsSameListener(napi_env env, const EventClassifyInfo &eventClassifyInfo, napi_value handler);

    void RemoveHandlerRef(napi_env env);

private:
    bool isOnce_ = false;
    std::string eventType_;
    napi_ref handlerRef_ = nullptr;
    std::mutex mutex_;
    EventClassifyInfo eventClassifyInfo_;
};

class SessionFuncHelper {
public:
    static std::string GetFuncName(uint32_t sessionType);

private:
    static std::map<uint32_t, std::string> sessionFuncMap_;
};
} // namespace OHOS::UpdateService
#endif // UPDATE_SESSION_H