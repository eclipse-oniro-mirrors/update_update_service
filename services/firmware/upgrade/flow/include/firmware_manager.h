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

#ifndef FIRMWARE_MANAGER_H
#define FIRMWARE_MANAGER_H

#include "singleton.h"

#include "business_error.h"
#include "constant.h"
#include "download_options.h"
#include "firmware_common.h"
#include "firmware_flow_manager.h"
#include "firmware_result_process.h"
#include "firmware_task.h"
#include "device_adapter.h"
#include "firmware_preferences_utils.h"
#include "schedule_task.h"
#include "upgrade_options.h"

namespace OHOS {
namespace UpdateService {
class FirmwareManager : public DelayedSingleton<FirmwareManager> {
    DECLARE_DELAYED_SINGLETON(FirmwareManager);

public:
    void Init(StartupReason startupReason);
    bool IsIdle();
    std::vector<ScheduleTask> GetScheduleTasks();
    bool Exit();

    void DoCheck(OnCheckComplete onCheckComplete);
    void DoDownload(const DownloadOptions &downloadOptions, BusinessError &businessError);
    void DoInstall(const UpgradeOptions &upgradeOptions, BusinessError &businessError, InstallType installType);
    void DoAutoDownloadSwitchChanged(bool isDownloadSwitchOn);
    void DoAutoUpgradeSwitchChanged(bool isNightUpgradeSwitchOn);
    void DoClearError(BusinessError &businessError);
    void DoCancel(BusinessError &businessError);
    void DoTerminateUpgrade(BusinessError &businessError);

private:
    void DelayInit(StartupReason startupReason);
    void RestoreUpdate();
    void NotifyInitEvent();
    void RegisterAllListeners();

    void HandleBootInstallOnStatusProcess(FirmwareTask &task);
    void HandleBootDownloadOnStatusProcess(FirmwareTask &task);
    void HandleBootDownloadedStatusProcess(FirmwareTask &task);

    bool CanInstall(void);
    ScheduleTask GetAutoUpgradeScheduleTask();

private:
    std::shared_ptr<FirmwarePreferencesUtil> preferencesUtil_ =
        DelayedSingleton<FirmwarePreferencesUtil>::GetInstance();
};
} // namespace UpdateService
} // namespace OHOS
#endif // FIRMWARE_MANAGER_H