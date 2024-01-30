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

#ifndef DUPDATE_INET_OBSERVER_H
#define DUPDATE_INET_OBSERVER_H

#include <functional>

#include "network_type.h"

namespace OHOS {
namespace UpdateEngine {
enum class NetChangeCallbackType {
    HOTA_BI_REPORT = 0,
    HOTA_DOWNLOAD = 1,
    PARAM_AUTO_UPGRADE = 2,
    COMMON_DOWNLOAD = 3,
};

using NetObserverCallback = std::function<void(NetType)>;

class INetObserverCallback {
public:
    virtual bool OnNetChange(NetType netType) = 0;
};

class DupdateINetObserver {
public:
    virtual void SetCallback(const std::weak_ptr<INetObserverCallback> &callback) = 0;
    virtual void StartObserver() = 0;
};
} // namespace UpdateEngine
} // namespace OHOS
#endif // DUPDATE_INET_OBSERVER_H