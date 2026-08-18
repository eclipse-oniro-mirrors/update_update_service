/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "firmware_event_manager.h"

#include "bell_utils.h"
#include "dupdate_net_manager.h"
#include "event_id.h"
#include "firmware_auto_upgrade_alarm_manager.h"
#include "firmware_auto_upgrade_mode.h"
#include "firmware_callback_utils.h"
#include "firmware_changelog_utils.h"
#include "firmware_constant.h"
#include "firmware_download_mode.h"
#include "firmware_flow_manager.h"
#include "firmware_iexecute_mode.h"
#include "firmware_log.h"
#include "firmware_manager.h"
#include "firmware_update_helper.h"
#include "progress_thread.h"
#include "string_utils.h"
#include "upgrade_status.h"

namespace OHOS::UpdateService {
FirmwareEventManager::FirmwareEventManager()
{
    FIRMWARE_LOGI("FirmwareEventManager");
}

FirmwareEventManager::~FirmwareEventManager()
{
    FIRMWARE_LOGI("~FirmwareEventManager");
}

void FirmwareEventManager::HandleEvent(CommonEventType event)
{
    FIRMWARE_LOGI("handleEvent event %{public}d", static_cast<uint32_t>(event));
    FirmwareAutoUpgradeAlarmManager::GetInstance()->RefreshAutoUpgradeAlarm(event);
    DistributeEventBusiness(event);
}

void FirmwareEventManager::DistributeEventBusiness(CommonEventType event)
{
    switch (event) {
        case CommonEventType::AUTO_UPGRADE:
            DoAutoUpgrade(event);
            break;
        case CommonEventType::NIGHT_UPGRADE:
            HandleNightUpgrade();
            break;
        case CommonEventType::NET_CHANGED:
            HandleNetChanged();
            break;
        case CommonEventType::BOOT_COMPLETE:
            HandleBootComplete();
            break;
        default:
            break;
    }
}

void FirmwareEventManager::DoAutoUpgrade(CommonEventType event)
{
    FIRMWARE_LOGI("DoAutoUpgrade");
    FirmwareTask task;
    FirmwareTaskOperator().QueryTask(task);
    FIRMWARE_LOGI("status is %{public}d", CAST_INT(task.status));
    if (task.isExistTask && task.status == UpgradeStatus::UPDATING) {
        FIRMWARE_LOGI("task status is UPDATING, not allow auto upgrade");
        return;
    }

    auto *flowManager = new FirmwareFlowManager();
    std::shared_ptr<FirmwareIExecuteMode> executeMode = std::make_shared<FirmwareAutoUpgradeMode>(
        [=]() {
            FIRMWARE_LOGI("FirmwareEventManager DoAutoUpgrade finish");
            delete flowManager;
        },
        event);
    flowManager->SetExecuteMode(executeMode);
    flowManager->Start();
}

void FirmwareEventManager::HandleNightUpgrade()
{
    if (!preferencesUtil_->GetNightUpgradeSwitch()) {
        FIRMWARE_LOGI("HandleNightUpgrade error, night upgrade switch is off");
        return;
    }
    FirmwareTask task;
    FirmwareTaskOperator firmwareTaskOperator;
    firmwareTaskOperator.QueryTask(task);
    if (!task.isExistTask) {
        return;
    }

    FIRMWARE_LOGI("HandleNightUpgrade status %{public}d", task.status);
    if (task.status != UpgradeStatus::DOWNLOAD_SUCCESS) {
        return;
    }
    firmwareTaskOperator.UpdateUpgradeModeByTaskId(task.taskId, UpgradeMode::NIGHT);
    DoNightUpgrade(task);
}

void FirmwareEventManager::DoNightUpgrade(const FirmwareTask &task)
{
    if (BellUtils::IsSupported() && BellUtils::GetBellStatus()) {
        if (preferencesUtil_->SaveBool(Firmware::RESTORE_BOOT_ANIMATION_SOUND, true)) {
            BellUtils::TurnOffBell();
            FIRMWARE_LOGI("Turn off boot animation sound before night upgrade");
        } else {
            FIRMWARE_LOGE("Save boot animation sound restore flag failed");
        }
    }

    BusinessError businessError;
    std::vector<ErrorMessage> errorMessages;
    UpgradeOptions options;
    options.order = Order::INSTALL_AND_APPLY;

    UpgradeOptions upgradeOptions;
    DelayedSingleton<FirmwareManager>::GetInstance()->DoInstall(upgradeOptions, businessError,
        FirmwareUpdateHelper::GetInstallType());
}

void FirmwareEventManager::HandleNetChanged()
{
    FIRMWARE_LOGI("HandleNetChanged");
#ifdef NETMANAGER_BASE_ENABLE
    FirmwareTask task;
    FirmwareTaskOperator().QueryTask(task);
    if (!task.isExistTask) {
        return;
    }
    FIRMWARE_LOGI("HandleNetChanged status %{public}d", task.status);
    if (!NetManager::GetInstance()->IsNetAvailable(task.downloadAllowNetwork)) {
        return;
    }

    if (task.status == UpgradeStatus::DOWNLOAD_PAUSE) {
        HandleResumeDownload(task);
        return;
    }

    if (task.status == UpgradeStatus::CHECK_VERSION_SUCCESS) {
        bool isDownloadSwitchOn = preferencesUtil_->GetAutoDownloadSwitch();
        FIRMWARE_LOGI("HandleNetChanged isDownloadSwitchOn %{public}s",
            StringUtils::GetBoolStr(isDownloadSwitchOn).c_str());
        if (isDownloadSwitchOn) {
            DoAutoDownload(task);
        }
    }
#endif
}

void FirmwareEventManager::HandleBootComplete()
{
    FIRMWARE_LOGI("HandleBootComplete");
    FirmwareTask task;
    FirmwareTaskOperator().QueryTask(task);
    if (!task.isExistTask) {
        FIRMWARE_LOGI("HandleBootComplete has no task");
        return;
    }

    FIRMWARE_LOGI("HandleBootComplete status %{public}d", CAST_INT(task.status));
    if (task.status == UpgradeStatus::UPDATING) {
        HandleBootUpdateOnStatusProcess(task);
        return;
    }

    // ab 升级安装完成
    if (task.status == UpgradeStatus::INSTALL_SUCCESS) {
        HandleBootUpdateSuccess(task);
        return;
    }
}

// updater调用后正常启动
void FirmwareEventManager::HandleBootUpdateOnStatusProcess(const FirmwareTask &task)
{
    FIRMWARE_LOGI("HandleBootUpdateOnStatusProcess");
    FirmwareResultProcess resultProcess;
    std::map<std::string, UpdateResult> resultMap;
    std::vector<FirmwareComponent> components;
    FirmwareComponentOperator().QueryAll(components);
    switch (resultProcess.GetUpdaterResult(components, resultMap)) {
        case UpdateResultCode::SUCCESS:
            HandleBootUpdateSuccess(task);
            break;
        case UpdateResultCode::FAILURE:
            HandleBootUpdateFail(task, resultMap);
            break;
        default:
            break;
    }
}

void FirmwareEventManager::HandleBootUpdateSuccess(const FirmwareTask &task)
{
    preferencesUtil_->SaveString(Firmware::UPDATE_ACTION, "upgrade");
    std::vector<FirmwareComponent> components;
    FirmwareComponentOperator().QueryAll(components);
    std::vector<VersionComponent> versionComponents;
    for (const auto &component : components) {
        VersionComponent versionComponent;
        versionComponent.componentType = CAST_INT(ComponentType::OTA);
        versionComponent.componentId = component.componentId;
        versionComponent.upgradeAction = UpgradeAction::UPGRADE;
        versionComponent.displayVersion = component.targetBlDisplayVersionNumber;
        versionComponent.innerVersion = component.targetBlVersionNumber;
        versionComponent.componentExtra = JsonBuilder().Append("{}").ToJson();
        versionComponents.push_back(versionComponent);
    }

    DelayedSingleton<FirmwareChangelogUtils>::GetInstance()->SaveHotaCurrentVersionComponentId();
    if (task.combinationType == CombinationType::HOTA) {
        FIRMWARE_LOGI("notify upgrade success");
        DelayedSingleton<FirmwareCallbackUtils>::GetInstance()->NotifyEvent(task.taskId, EventId::EVENT_UPGRADE_SUCCESS,
            UpgradeStatus::UPDATE_SUCCESS, ErrorMessage{}, versionComponents);
        FirmwareUpdateHelper::ClearFirmwareInfo();
        return;
    }
}

void FirmwareEventManager::HandleBootUpdateFail(const FirmwareTask &task,
    const std::map<std::string, UpdateResult> &resultMap)
{
    preferencesUtil_->SaveString(Firmware::UPDATE_ACTION, "recovery");
    std::vector<FirmwareComponent> components;
    FirmwareComponentOperator().QueryAll(components);
    std::vector<VersionComponent> versionComponents;
    for (const auto &component : components) {
        VersionComponent versionComponent;
        versionComponent.componentType = CAST_INT(ComponentType::OTA);
        versionComponent.componentId = component.componentId;
        versionComponent.upgradeAction = UpgradeAction::UPGRADE;
        versionComponent.displayVersion = component.targetBlDisplayVersionNumber;
        versionComponent.innerVersion = component.targetBlVersionNumber;
        versionComponent.componentExtra = JsonBuilder().Append("{}").ToJson();
        versionComponents.push_back(versionComponent);
    }

    ErrorMessage errorMessage;
    for (const auto &result : resultMap) {
        if (result.second.result == UPDATER_RESULT_FAILURE) {
            errorMessage.errorCode = CAST_INT(result.second.GetUpdateResultCode());
            errorMessage.errorMessage = result.second.reason;
            break;
        }
    }

    DelayedSingleton<FirmwareCallbackUtils>::GetInstance()->NotifyEvent(task.taskId, EventId::EVENT_UPGRADE_FAIL,
        UpgradeStatus::UPDATE_FAIL, errorMessage, versionComponents);
    FIRMWARE_LOGI("upgrade fail");
    FirmwareUpdateHelper::ClearFirmwareInfo();
}

void FirmwareEventManager::DoAutoDownload(const FirmwareTask &task)
{
    FIRMWARE_LOGI("DoAutoDownload");
    HandleResumeDownload(task);
}

void FirmwareEventManager::HandleResumeDownload(const FirmwareTask &task)
{
    BusinessError businessError;
    DownloadOptions downloadOptions;
    downloadOptions.allowNetwork = task.downloadAllowNetwork;
    auto *flowManager = new FirmwareFlowManager();
    std::shared_ptr<FirmwareIExecuteMode> executeMode =
        std::make_shared<FirmwareDownloadMode>(downloadOptions, businessError, [=]() mutable {
            FIRMWARE_LOGI("FirmwareManager DoDownload finish");
            delete flowManager;
            flowManager = nullptr;
        });
    flowManager->SetExecuteMode(executeMode);
    flowManager->Start();
}

void FirmwareEventManager::DoCheckSuccessAutoDownload(const FirmwareTask &task)
{
    FIRMWARE_LOGD("DoCheckSuccessAutoDownload not support");
}
} // namespace OHOS::UpdateService
