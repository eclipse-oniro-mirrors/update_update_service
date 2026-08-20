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

#ifndef UPDATE_NOTIFY_H
#define UPDATE_NOTIFY_H

#include <condition_variable>
#include <mutex>
#include <string>
#include "ability_connect_callback_stub.h"
#include "ability_connect_callback_interface.h"
#include "ability_manager_interface.h"
#include "ability_manager_client.h"
#include "i_update_notify.h"
#include "want.h"

namespace OHOS {
namespace UpdateService {
class NotifyConnection;
class UpdateNotify : public IRemoteStub<IUpdateNotify> {
public:
    DISALLOW_COPY_AND_MOVE(UpdateNotify);

    UpdateNotify();
    ~UpdateNotify();
    static sptr<UpdateNotify> GetInstance();
    bool ConnectToAppService(const std::string &eventInfo, const std::string &subscribeInfo);
    bool ConnectAbility(const sptr<NotifyConnection> &connect);
    sptr<IRemoteObject> GetRemoteConnectObj();

private:
    bool HandleMessage(const std::string &message);
    ErrCode ConnectAbility(const AAFwk::Want &want, const sptr<AAFwk::AbilityConnectionStub> &connect);
    ErrCode DisconnectAbility(const sptr<AAFwk::AbilityConnectionStub> &connect);

private:
    static std::mutex instanceLock_;
    static sptr<UpdateNotify> instance_;
    std::mutex connectLock_;
    sptr<NotifyConnection> connectionStub_ = nullptr;

    enum class UpdateAppCode {
        UNKNOWN = 0,
        UPDATE_APP = 5
    };
};

class NotifyConnection : public AAFwk::AbilityConnectionStub {
public:
    explicit NotifyConnection() = default;
    ~NotifyConnection() override = default;
    sptr<IRemoteObject> GetRemoteObj();
    bool IsConnected();

    void OnAbilityConnectDone(const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject,
        int32_t resultCode) override;
    void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode) override;

private:
    sptr<IRemoteObject> remoteObject_ = nullptr;
    std::mutex connectMutex_;
    std::condition_variable conditionVar_;
};
} // namespace UpdateService
} // namespace OHOS
#endif // UPDATE_NOTIFY_H