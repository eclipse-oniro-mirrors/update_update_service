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

#include "dupdate_net_observer.h"

#include <pthread.h>
#include <thread>
#include <unistd.h>

#include "net_conn_client.h"
#include "net_conn_constants.h"

#include "string_utils.h"
#include "update_log.h"

using namespace OHOS::NetManagerStandard;

namespace OHOS {
namespace UpdateService {
void NetObserver::SetCallback(const std::weak_ptr<INetObserverCallback> &callback)
{
    callback_ = callback;
}

void NetObserver::StartObserver()
{
    ENGINE_LOGI("StartObserver");
    auto weakSelf = std::weak_ptr<NetObserver>(shared_from_this());
    auto observerExecFunc = [weakSelf]() {
        auto sharedSelf = weakSelf.lock();
        if (!sharedSelf) {
            ENGINE_LOGE("NetObserver already destroyed, abandon register");
            return;
        }
        NetSpecifier netSpecifier;
        NetAllCapabilities netAllCapabilities;
        netAllCapabilities.netCaps_.insert(NetManagerStandard::NetCap::NET_CAPABILITY_INTERNET);
        netSpecifier.ident_ = "";
        netSpecifier.netCapabilities_ = netAllCapabilities;
        sptr<NetSpecifier> specifier = new NetSpecifier(netSpecifier);
        constexpr int32_t RETRY_MAX_TIMES = 10;
        int32_t retryCount = 0;
        int32_t ret = NetConnResultCode::NET_CONN_SUCCESS;
        do {
            ret = NetConnClient::GetInstance().RegisterNetConnCallback(specifier, sharedSelf.get(), 0);
            if (ret == NetConnResultCode::NET_CONN_SUCCESS) {
                ENGINE_LOGI("StartObserver register success");
                return;
            }
            retryCount++;
            ENGINE_LOGE("StartObserver retry, ret = %{public}d", ret);
            sleep(1);
        } while (retryCount < RETRY_MAX_TIMES);
        ENGINE_LOGE("StartObserver failed");
    };
    std::thread th = std::thread(observerExecFunc);
    th.detach();
}

int32_t NetObserver::NetCapabilitiesChange(sptr<NetHandle> &netHandle, const sptr<NetAllCapabilities> &netAllCap)
{
    if (netAllCap == nullptr) {
        return 0;
    }
    if (netAllCap->netCaps_.count(NET_CAPABILITY_INTERNET) <= 0) {
        OnNetChange(NetType::NO_NET);
        return 0;
    }
    ENGINE_LOGI("NetCapabilitiesChange NetAvailable");
    if (netAllCap->bearerTypes_.count(BEARER_ETHERNET) > 0 || netAllCap->bearerTypes_.count(BEARER_WIFI) > 0) {
        // wifi和以太网都映射为wifi
        OnNetChange(NetType::NOT_METERED_WIFI);
        return 0;
    }
    if (netAllCap->bearerTypes_.count(BEARER_WIFI_AWARE) > 0) {
        OnNetChange(NetType::METERED_WIFI);
        return 0;
    }
    if (netAllCap->bearerTypes_.count(BEARER_CELLULAR) > 0) {
        OnNetChange(NetType::CELLULAR);
        return 0;
    }
    return 0;
}

int32_t NetObserver::NetLost(sptr<NetHandle> &netHandle)
{
    ENGINE_LOGI("NetLost");
    OnNetChange(NetType::NO_NET);
    return 0;
}

void NetObserver::OnNetChange(NetType netType)
{
    ENGINE_LOGI("OnNetChange %{public}d", netType);
    if (callback_.expired()) {
        ENGINE_LOGI("callback is recycled");
        return;
    }

    const auto netManager = callback_.lock();
    if (netManager == nullptr) {
        ENGINE_LOGI("callback is empty");
        return;
    }
    bool result = netManager->OnNetChange(netType);
    ENGINE_LOGD("OnNetChange callback result %{public}s", StringUtils::GetBoolStr(result).c_str());
}
} // namespace UpdateService
} // namespace OHOS