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

#include "firmware_update_helper.h"

namespace OHOS {
namespace UpdateService {
std::string FirmwareUpdateHelper::BuildTaskId(const std::vector<FirmwareComponent> &componentList)
{
    if (componentList.empty()) {
        return "";
    }
    std::string taskId;
    for (const FirmwareComponent &component : componentList) {
        if (!taskId.empty()) {
            taskId.append("_");
        }
        taskId.append(component.versionId);
    }
    return Sha256Utils::CalculateHashCode(taskId);
}

void FirmwareUpdateHelper::ClearFirmwareInfo()
{
    FIRMWARE_LOGI("ClearFirmwareInfo");

    FirmwareTask task;
    FirmwareTaskOperator firmwareTaskOperator;
    firmwareTaskOperator.QueryTask(task);
    firmwareTaskOperator.DeleteAll();
    FirmwareComponentOperator().DeleteAll();
    FirmwareFileUtils::DeleteDownloadFiles();
}

// 此处为桩函数，默认电量足够。
bool FirmwareUpdateHelper::IsBatteryEnough(int32_t batteryLimit)
{
    return true;
}

InstallType FirmwareUpdateHelper::GetInstallType()
{
    InstallType installType = InstallType::QUICK_FIX;
    if (DeviceAdapter::GetBootSlot() == Firmware::BOOTSLOTS_AB_UPDATE_VALUE) {
        installType = IsStreamUpgrade() ? InstallType::STREAM_INSTALLLER : InstallType::SYS_INSTALLER;
    } else {
        installType = InstallType::UPDATER;
    }
    FIRMWARE_LOGI("get installType(bootsloot) %{public}d", CAST_INT(installType));
    return installType;
}

bool FirmwareUpdateHelper::IsUpgradeFailed(UpgradeStatus status)
{
    return status == UpgradeStatus::DOWNLOAD_FAIL ||
           status == UpgradeStatus::PACKAGE_TRANS_FAIL ||
           status == UpgradeStatus::INSTALL_FAIL ||
           status == UpgradeStatus::UPDATE_FAIL ||
           status == UpgradeStatus::VERIFY_FAIL;
}

VersionComponent FirmwareUpdateHelper::BuildHotaVersionComponent(std::vector<FirmwareComponent> &hotaComponents)
{
    VersionComponent hotaVersionComponent;
    hotaVersionComponent.componentId = hotaComponents[0].componentId;
    hotaVersionComponent.upgradeAction = UpgradeAction::UPGRADE;
    hotaVersionComponent.componentType = CAST_INT(ComponentType::OTA);
    InstallType installType = FirmwareUpdateHelper::GetInstallType();
    size_t effectiveMode;
    if (installType == InstallType::SYS_INSTALLER || installType == InstallType::STREAM_INSTALLLER) {
        effectiveMode = static_cast<size_t>(EffectiveMode::LIVE_AND_COLD);
    } else {
        effectiveMode = static_cast<size_t>(EffectiveMode::COLD);
    }
    hotaVersionComponent.effectiveMode = effectiveMode;
    size_t otaMode;
    if (installType == InstallType::SYS_INSTALLER) {
        otaMode = static_cast<size_t>(OtaMode::AB_REGULAR_OTA);
    } else if (installType == InstallType::STREAM_INSTALLLER) {
        otaMode = static_cast<size_t>(OtaMode::AB_STREAM_OTA);
    } else {
        otaMode = static_cast<size_t>(OtaMode::REGULAR_OTA);
    }
    hotaVersionComponent.otaMode = otaMode;
    hotaVersionComponent.innerVersion = hotaComponents[0].targetBlVersionNumber;
    hotaVersionComponent.displayVersion = hotaComponents[0].targetBlDisplayVersionNumber;
    for (const auto &component : hotaComponents) {
        hotaVersionComponent.size += static_cast<size_t>(component.size);
    }
    return hotaVersionComponent;
}

void FirmwareUpdateHelper::BuildNewVersionInfo(const std::vector<FirmwareComponent> &components,
    std::vector<VersionComponent> &versionComponents)
{
    if (components.empty()) {
        FIRMWARE_LOGI("BuildNewVersionInfo component is null");
        return;
    }
    std::vector<FirmwareComponent> hotaComponents;
    for (auto &component : components) {
        if (component.blVersionType == CAST_INT(BlType::HOTA_TYPE)) {
            hotaComponents.push_back(component);
        }
        if (component.blVersionType == CAST_INT(BlType::DYNAMIC_TYPE)) {
            hotaComponents.push_back(component);
        }
    }

    if (!hotaComponents.empty()) {
        VersionComponent hotaVersionComponent = BuildHotaVersionComponent(hotaComponents);
        versionComponents.push_back(hotaVersionComponent);
    }
}

void FirmwareUpdateHelper::BuildCurrentVersionInfo(CurrentVersionInfo &currentVersionInfo)
{
    currentVersionInfo.osVersion = DeviceAdapter::GetOsVersion();
    currentVersionInfo.deviceName = DeviceAdapter::GetDeviceName();

    std::shared_ptr<FirmwarePreferencesUtil> utils = DelayedSingleton<FirmwarePreferencesUtil>::GetInstance();
    VersionComponent hotaVersionComponent;
    hotaVersionComponent.displayVersion = DeviceAdapter::GetDisplayVersion();
    hotaVersionComponent.innerVersion = DeviceAdapter::GetRealVersion();
    hotaVersionComponent.descriptionInfo.descriptionType = DescriptionType::ID;
    hotaVersionComponent.componentId = utils->ObtainString(Firmware::HOTA_CURRENT_COMPONENT_ID, "");
    hotaVersionComponent.componentType = CAST_INT(ComponentType::OTA);
    currentVersionInfo.versionComponents.push_back(hotaVersionComponent);
}

bool FirmwareUpdateHelper::IsUpgradePackagesReady(const std::vector<FirmwareComponent> &componentList)
{
    if (componentList.empty()) {
        FIRMWARE_LOGE("IsUpgradePackagesReady: componentList is null!");
        return false;
    }

    return std::all_of(componentList.begin(), componentList.end(), [](const FirmwareComponent &component) {
        if (!FileUtils::IsFileExist(component.spath)) {
            FIRMWARE_LOGE("IsUpgradePackagesReady: package [%{public}s] is not exist!", component.spath.c_str());
            return false;
        }
        int64_t realSize = FileUtils::RetrieveFileSize(component.spath);
        if (realSize != component.size) {
            FIRMWARE_LOGE("IsUpgradePackagesReady: calculate component size %{public}s != DB component size %{public}s",
                std::to_string(realSize).c_str(), std::to_string(component.size).c_str());
            return false;
        }
        return true;
    });
}

bool FirmwareUpdateHelper::IsStreamUpgrade()
{
    std::vector<FirmwareComponent> components;
    FirmwareComponentOperator().QueryAll(components);
    return IsStreamUpgrade(components);
}

bool FirmwareUpdateHelper::IsStreamUpgrade(const std::vector<FirmwareComponent> &componentList)
{
    if (componentList.empty()) {
        FIRMWARE_LOGI("FirmwareUpdateHelper::IsStreamUpgrade componentList is empty");
        return false;
    }
    return (componentList[0].otaType == OtaMode::AB_STREAM_OTA);
}

} // namespace UpdateService
} // namespace OHOS