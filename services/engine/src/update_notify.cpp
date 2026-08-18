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

#include "update_notify.h"

#include "cJSON.h"
#include "extension_manager_client.h"
#include "iservice_registry.h"

#include "subscribe_info.h"
#include "update_log.h"

namespace OHOS {
namespace UpdateService {
std::mutex UpdateNotify::instanceLock_;
sptr<UpdateNotify> UpdateNotify::instance_ = nullptr;

UpdateNotify::UpdateNotify()
{
    ENGINE_LOGD("UpdateNotify");
}

UpdateNotify::~UpdateNotify()
{
    ENGINE_LOGD("~UpdateNotify");
}

sptr<UpdateNotify> UpdateNotify::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> autoLock(instanceLock_);
        if (instance_ == nullptr) {
            instance_ = new UpdateNotify();
        }
    }
    return instance_;
}

ErrCode UpdateNotify::ConnectAbility(const AAFwk::Want &want, const sptr<AAFwk::AbilityConnectionStub> &connect)
{
    ErrCode result =
        AAFwk::ExtensionManagerClient::GetInstance().ConnectServiceExtensionAbility(want, connect, nullptr, -1);
    ENGINE_LOGI("ConnectAbility result %{public}d", result);
    return result;
}

ErrCode UpdateNotify::DisconnectAbility(const sptr<AAFwk::AbilityConnectionStub> &connect)
{
    ErrCode result =
        AAFwk::ExtensionManagerClient::GetInstance().DisconnectAbility(connect);
    ENGINE_LOGI("DisconnectAbility result %{public}d", result);
    return result;
}

bool UpdateNotify::ConnectToAppService(const std::string &eventInfo, const std::string &subscribeInfo)
{
    if (eventInfo.empty()) {
        ENGINE_LOGE("ConnectToAppService eventInfo error.");
        return false;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == nullptr) {
        ENGINE_LOGE("ConnectToAppService create cJSON root failed");
        return false;
    }

    cJSON *eventInfoObj = cJSON_Parse(eventInfo.c_str());
    cJSON_AddItemToObject(root, "EventInfo", eventInfoObj ? eventInfoObj : cJSON_CreateNull());
    cJSON *subscribeInfoObj = cJSON_Parse(subscribeInfo.c_str());
    cJSON_AddItemToObject(root, "SubscribeInfo", subscribeInfoObj ? subscribeInfoObj : cJSON_CreateNull());

    char *data = cJSON_PrintUnformatted(root);
    if (data == nullptr) {
        cJSON_Delete(root);
        ENGINE_LOGE("ConnectToAppService cJSON_PrintUnformatted failed");
        return false;
    }
    std::string message = std::string(data);
    cJSON_free(data);
    cJSON_Delete(root);
    return HandleMessage(message);
}

bool UpdateNotify::HandleMessage(const std::string &message)
{
    std::string bundleName = UPDATE_APP_PACKAGE_NAME;
    std::string abilityName = UPDATE_APP_SERVICE_EXT_ABILITY_NAME;
    AAFwk::Want want;
    want.SetElementName(bundleName, abilityName);
    want.SetParam("Timeout", UPDATE_APP_TIMEOUT);

    auto notifyContext = sptr<NotifyConnectContext>::MakeSptr();
    notifyContext->state = ConnectState::WAITING;
    auto connect = sptr<NotifyConnection>::MakeSptr(notifyContext);

    ErrCode ret = ConnectAbility(want, connect);
    if (ret != OHOS::ERR_OK) {
        ENGINE_LOGE("HandleMessage connect failed immediately");
        DisconnectAbility(connect);
        return false;
    }

    std::unique_lock<std::mutex> uniqueLock(notifyContext->mtx);
    bool waitOk = notifyContext->conditionVar.wait_for(uniqueLock, std::chrono::seconds(UPDATE_APP_CONNECT_TIMEOUT),
        [ctx = notifyContext]() { return ctx->state == ConnectState::SUCCESS || ctx->state == ConnectState::FAILED; });
    if (!waitOk || notifyContext->state != ConnectState::SUCCESS || notifyContext->remote == nullptr) {
        uniqueLock.unlock();
        ENGINE_LOGE("HandleMessage connect fail, state:%{public}d", static_cast<int>(notifyContext->state));
        DisconnectAbility(connect);
        return false;
    }
    sptr<IRemoteObject> remote = notifyContext->remote;
    uniqueLock.unlock();

    MessageParcel data;
    if (!data.WriteString16(Str8ToStr16(message))) {
        ENGINE_LOGE("HandleMessage, write message failed");
        DisconnectAbility(connect);
        return false;
    }

    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    int32_t result = remote->SendRequest(CAST_INT(UpdateAppCode::UPDATE_APP), data, reply, option);
    DisconnectAbility(connect);
    if (result != 0) {
        ENGINE_LOGE("HandleMessage SendRequest, error result %{public}d", result);
        return false;
    }

    return true;
}

NotifyConnection::NotifyConnection(sptr<NotifyConnectContext> connectContext) : connectContext_(connectContext)
{
    ENGINE_LOGD("NotifyConnection constructor");
}

void NotifyConnection::OnAbilityConnectDone(const AppExecFwk::ElementName &element,
    const sptr<IRemoteObject> &remoteObject, int32_t resultCode)
{
    ENGINE_LOGI("OnAbilityConnectDone successfully. result %{public}d", resultCode);
    if (connectContext_ == nullptr) {
        ENGINE_LOGE("connectContext_ is nullptr");
        return;
    }
    std::lock_guard<std::mutex> lock(connectContext_->connectMutex);
    if (connectContext_->state != ConnectState::WAITING) {
        ENGINE_LOGE("OnAbilityConnectDone skip, task finished already");
        return;
    }
    if (resultCode != ERR_OK) {
        ENGINE_LOGE("ability connect failed, error code: %{public}d", resultCode);
        connectContext_->state = ConnectState::FAILED;
        connectContext_->conditionVar.notify_one();
        return;
    }

    if (remoteObject == nullptr) {
        ENGINE_LOGE("remoteObject is nullptr");
        connectContext_->state = ConnectState::FAILED;
        connectContext_->conditionVar.notify_one();
        return;
    }
    connectContext_->remote = remoteObject;
    connectContext_->state = ConnectState::SUCCESS;
    connectContext_->conditionVar.notify_one();
}

void NotifyConnection::OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode)
{
    ENGINE_LOGI("OnAbilityDisconnectDone successfully. result %{public}d", resultCode);
    if (connectContext_ != nullptr) {
        std::lock_guard<std::mutex> lock(connectContext_->connectMutex);
        connectContext_->remote = nullptr;
    }
}
} // namespace UpdateService
} // namespace OHOS