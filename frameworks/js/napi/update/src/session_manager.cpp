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

#include "session_manager.h"

#include "napi/native_node_api.h"
#include "node_api.h"

#include "client_helper.h"
#include "update_define.h"
#include "update_session.h"

using namespace std;

namespace OHOS::UpdateService {
SessionManager::SessionManager(napi_env env, napi_ref thisReference) : env_(env), thisReference_(thisReference)
{
    ENGINE_LOGI("SessionManager constructor");
}

SessionManager::~SessionManager()
{
    ENGINE_LOGI("SessionManager destructor");
    if (thisReference_ != nullptr) {
        napi_delete_reference(env_, thisReference_);
        thisReference_ = nullptr;
    }
}

void SessionManager::AddSession(std::shared_ptr<BaseSession> session)
{
    PARAM_CHECK(session != nullptr, return, "Invalid param");
    std::lock_guard<std::recursive_mutex> guard(sessionMutex_);
    sessions_.insert(make_pair(session->GetSessionId(), session));
}

void SessionManager::RemoveSession(uint32_t sessionId)
{
    std::lock_guard<std::recursive_mutex> guard(sessionMutex_);
    sessions_.erase(sessionId);
}

bool SessionManager::GetFirstSessionId(uint32_t &sessionId)
{
    std::lock_guard<std::recursive_mutex> guard(sessionMutex_);
    {
        if (sessions_.empty()) {
            return false;
        }
        sessionId = sessions_.begin()->second->GetSessionId();
        return true;
    }
}

bool SessionManager::GetNextSessionId(uint32_t &sessionId)
{
    std::lock_guard<std::recursive_mutex> guard(sessionMutex_);
    {
        auto iter = sessions_.find(sessionId);
        if (iter == sessions_.end()) {
            return false;
        }
        iter++;
        if (iter == sessions_.end()) {
            return false;
        }
        sessionId = iter->second->GetSessionId();
    }
    return true;
}

int32_t SessionManager::ProcessUnsubscribe(const std::string &eventType, size_t argc, napi_value arg)
{
    napi_handle_scope scope;
    napi_status status = napi_open_handle_scope(env_, &scope);
    PARAM_CHECK(status == napi_ok, return -1, "Error open handle");

    uint32_t nextSessId = 0;
    bool hasNext = GetFirstSessionId(nextSessId);
    while (hasNext) {
        std::lock_guard<std::recursive_mutex> guard(sessionMutex_);
        uint32_t currSessId = nextSessId;
        auto iter = sessions_.find(currSessId);
        if (iter == sessions_.end()) {
            break;
        }
        hasNext = GetNextSessionId(nextSessId);

        auto listener = (std::shared_ptr<UpdateListener> &)(iter->second);
        if (listener == nullptr) {
            iter = sessions_.erase(iter);
            continue;
        }
        if (listener->GetType() != SessionType::SESSION_SUBSCRIBE ||
            eventType.compare(listener->GetEventType()) != 0) {
            continue;
        }
        ENGINE_LOGI("ProcessUnsubscribe remove session");
        if (argc == 1) {
            listener->RemoveHandlerRef(env_);
            RemoveSession(currSessId);
        } else if (listener->CheckEqual(env_, arg, eventType)) {
            listener->RemoveHandlerRef(env_);
            RemoveSession(currSessId);
            break;
        }
    }
    napi_close_handle_scope(env_, scope);
    return 0;
}

void SessionManager::Unsubscribe(const EventClassifyInfo &eventClassifyInfo, napi_value handle)
{
    std::lock_guard<std::recursive_mutex> guard(sessionMutex_);
    for (auto iter = sessions_.begin(); iter != sessions_.end();) {
        if (iter->second == nullptr) {
            iter = sessions_.erase(iter); // erase nullptr
            continue;
        }

        if (iter->second->GetType() != SessionType::SESSION_SUBSCRIBE) {
            ++iter;
            continue;
        }

        auto listener = (std::shared_ptr<UpdateListener> &)(iter->second);
        if (handle == nullptr && listener->IsSubscribeEvent(eventClassifyInfo)) {
            ENGINE_LOGI("Unsubscribe, remove session %{public}d without handle", listener->GetSessionId());
            listener->RemoveHandlerRef(env_);
            iter = sessions_.erase(iter);
            continue;
        }

        if (listener->IsSameListener(env_, eventClassifyInfo, handle)) {
            ENGINE_LOGI("Unsubscribe, remove session %{public}d", listener->GetSessionId());
            listener->RemoveHandlerRef(env_);
            iter = sessions_.erase(iter);
            continue;
        }

        ++iter;
    }
}

BaseSession *SessionManager::FindSessionByHandle(napi_env env, const std::string &eventType, napi_value arg)
{
    uint32_t nextSessId = 0;
    bool hasNext = GetFirstSessionId(nextSessId);
    while (hasNext) {
        uint32_t currSessId = nextSessId;
        auto iter = sessions_.find(currSessId);
        if (iter == sessions_.end()) {
            break;
        }
        hasNext = GetNextSessionId(nextSessId);

        auto listener = (std::shared_ptr<UpdateListener> &)(iter->second);
        if (listener == nullptr) {
            iter = sessions_.erase(iter);
            continue;
        }
        if (listener->GetType() != SessionType::SESSION_SUBSCRIBE) {
            continue;
        }
        if ((eventType.compare(listener->GetEventType()) == 0) && listener->CheckEqual(env_, arg, eventType)) {
            return listener.get();
        }
    }
    return nullptr;
}

BaseSession *SessionManager::FindSessionByHandle(napi_env env, const EventClassifyInfo &eventClassifyInfo,
    napi_value arg)
{
    std::lock_guard<std::recursive_mutex> guard(sessionMutex_);
    for (auto &iter : sessions_) {
        if (iter.second == nullptr) {
            continue;
        }

        if (iter.second->GetType() != SessionType::SESSION_SUBSCRIBE) {
            continue;
        }

        auto listener = (std::shared_ptr<UpdateListener> &)(iter.second);
        if (listener->IsSameListener(env, eventClassifyInfo, arg)) {
            return listener.get();
        }
    }
    return nullptr;
}

void SessionManager::PublishToJS(const EventClassifyInfo &eventClassifyInfo, const EventInfo &eventInfo)
{
    napi_handle_scope scope;
    napi_status status = napi_open_handle_scope(env_, &scope);
    PARAM_CHECK_NAPI_CALL(env_, status == napi_ok, return, "Error open_handle_scope");
    napi_value thisVar = nullptr;
    status = napi_get_reference_value(env_, thisReference_, &thisVar);
    PARAM_CHECK_NAPI_CALL(env_, status == napi_ok, napi_close_handle_scope(env_, scope); return,
        "Error get_reference");

    std::lock_guard<std::recursive_mutex> guard(sessionMutex_);
    for (auto &iter : sessions_) {
        if (iter.second == nullptr) {
            continue;
        }

        if (iter.second->GetType() != SessionType::SESSION_SUBSCRIBE) {
            continue;
        }

        auto listener = (std::shared_ptr<UpdateListener> &)(iter.second);
        if (!listener->IsSubscribeEvent(eventClassifyInfo)) {
            continue;
        }

        listener->NotifyJS(env_, thisVar, eventInfo);
    }
    napi_close_handle_scope(env_, scope);
}

void SessionManager::Emit(const EventClassifyInfo &eventClassifyInfo, const EventInfo &eventInfo)
{
    ENGINE_LOGI("SessionManager::Emit 0x%{public}x", CAST_INT(eventClassifyInfo.eventClassify));
    auto task = [eventClassifyInfo, eventInfo, this]() { PublishToJS(eventClassifyInfo, eventInfo); };
    if (napi_status::napi_ok != napi_send_event(env_, task, napi_eprio_high)) {
        ENGINE_LOGE("PublishToJS: Failed to SendEvent");
    }
}
} // namespace OHOS::UpdateService