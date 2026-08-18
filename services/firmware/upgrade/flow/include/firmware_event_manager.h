/*
* Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef FIRMWARE_EVENT_MANAGER_H
#define FIRMWARE_EVENT_MANAGER_H

#include "singleton.h"

#include <condition_variable>
#include <mutex>

#include "constant.h"
#include "firmware_task.h"
#include "firmware_preferences_utils.h"
#include "firmware_result_process.h"

namespace OHOS::UpdateService {
class FirmwareEventManager : public DelayedSingleton<FirmwareEventManager> {
    DECLARE_DELAYED_SINGLETON(FirmwareEventManager);

public:
    void HandleEvent(CommonEventType event);
    void DoAutoUpgrade(CommonEventType event);

private:
    void DistributeEventBusiness(CommonEventType event);
    void HandleNightUpgrade();
    void HandleBootComplete();
    void HandleNetChanged();

    void HandleResumeDownload(const FirmwareTask &task);
    void DoCheckSuccessAutoDownload(const FirmwareTask &task);
    void DoAutoDownload(const FirmwareTask &task);

    void HandleBootUpdateSuccess(const FirmwareTask &task);
    void HandleBootUpdateFail(const FirmwareTask &task, const std::map<std::string, UpdateResult> &resultMap);
    void HandleBootUpdateOnStatusProcess(const FirmwareTask &task);
    void DoNightUpgrade(const FirmwareTask &task);

private:
    std::shared_ptr<FirmwarePreferencesUtil> preferencesUtil_ = FirmwarePreferencesUtil::GetInstance();
};
}
#endif // FIRMWARE_EVENT_MANAGER_H