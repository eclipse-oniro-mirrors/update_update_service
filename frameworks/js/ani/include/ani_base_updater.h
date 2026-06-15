/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANI_BASE_UPDATER_H
#define ANI_BASE_UPDATER_H

#include <mutex>
#include <vector>

#include "ohos.update.proj.hpp"
#include "taihe/callback.hpp"

#include "business_error.h"
#include "event_info.h"

namespace OHOS::UpdateService {
class AniBaseUpdater {
public:
    virtual ~AniBaseUpdater();
    AniBaseUpdater() = default;

    void On(taihe::callback_view<void(const ohos::update::EventInfo &)> callback);
    void Off(const taihe::callback<void(const ohos::update::EventInfo &)> &callback);
    void OffAll();

protected:
    virtual void RegisterCallback() {}
    virtual std::string GetPermissionName();
    virtual void UnRegisterCallback() {}
    bool SetError(int32_t ret, const std::string &funcName, const BusinessError &error);
    void CallbackEventInfo(const EventInfo &eventInfo);

private:
    BusinessError GetIpcBusinessError(const std::string &funcName, int32_t ipcRequestCode);
    void GetIpcErrorMessage(const std::string &funcName, int32_t ipcRequestCode,
        const std::string &callResultStr, std::string &msg);
    bool IsCommonError(CallResult callResult);
    int32_t ConvertToErrorCode(CallResult callResult);

    std::mutex mutex_;
    std::vector<taihe::callback<void(const ohos::update::EventInfo &)>> callbacks_;
};
}

#endif // ANI_BASE_UPDATER_H
