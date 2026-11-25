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

#include "ani_common_conveter.h"

namespace OHOS::UpdateService {
ohos::update::ErrorMessage AniCommonConverter::Converter(const ErrorMessage &error)
{
    return {
        .errorCode = static_cast<int32_t>(error.errorCode),
        .errorMessage = error.errorMessage
    };
}

taihe::array<ohos::update::ErrorMessage> AniCommonConverter::Converter(const std::vector<ErrorMessage> &errors)
{
    std::vector<ohos::update::ErrorMessage> taiheErrorMessage;
    taiheErrorMessage.reserve(errors.size());
    for (const auto &error : errors) {
        taiheErrorMessage.push_back(Converter(error));
    }
    return { taiheErrorMessage };
}

ohos::update::TaskInfo AniCommonConverter::Converter(const TaskInfo &info)
{
    return {
        .existTask = info.existTask,
        .taskBody = Converter(info.taskBody)
    };
}

ohos::update::CurrentVersionInfo AniCommonConverter::Converter(const CurrentVersionInfo &info)
{
    return {
        .osVersion = info.osVersion,
        .deviceName = info.deviceName,
        .versionComponents = Converter(info.versionComponents)
    };
}

ohos::update::NewVersionInfo AniCommonConverter::Converter(const NewVersionInfo &info)
{
    return {
        .versionDigestInfo = Converter(info.versionDigestInfo),
        .versionComponents = Converter(info.versionComponents)
    };
}

ohos::update::CheckResult AniCommonConverter::Converter(const CheckResult &result)
{
    return {
        .isExistNewVersion = result.isExistNewVersion,
        .newVersionInfo = Converter(result.newVersionInfo)
    };
}

ohos::update::TaskBody AniCommonConverter::Converter(const TaskBody &taskBody)
{
    ohos::update::TaskBody value = {
        .versionDigestInfo = Converter(taskBody.versionDigestInfo),
        .status = Converter(taskBody.status),
        .subStatus = static_cast<int32_t>(taskBody.subStatus),
        .progress = taskBody.progress,
        .installMode = static_cast<int32_t>(taskBody.installMode),
        .errorMessages = Converter(taskBody.errorMessages),
        .versionComponents = Converter(taskBody.versionComponents)
    };
    return value;
}

ohos::update::VersionDigestInfo AniCommonConverter::Converter(const VersionDigestInfo &versionDigestInfo)
{
    return {
        .versionDigest = versionDigestInfo.versionDigest
    };
}

ohos::update::UpgradeStatus AniCommonConverter::Converter(const UpgradeStatus &status)
{
    return ohos::update::UpgradeStatus::from_value(static_cast<int32_t>(status));
}

ohos::update::ComponentType AniCommonConverter::Converter(const ComponentType &componentType)
{
    return ohos::update::ComponentType::from_value(static_cast<int32_t>(componentType));
}

ohos::update::DescriptionType AniCommonConverter::Converter(const DescriptionType &descriptionType)
{
    return ohos::update::DescriptionType::from_value(static_cast<int32_t>(descriptionType));
}

ohos::update::EffectiveMode AniCommonConverter::Converter(const EffectiveMode &effectiveMode)
{
    return ohos::update::EffectiveMode::from_value(static_cast<int32_t>(effectiveMode));
}

ohos::update::DescriptionInfo AniCommonConverter::Converter(const DescriptionInfo &description)
{
    return {
        .descriptionType = Converter(description.descriptionType),
        .content = description.content
    };
}

ohos::update::UpgradeAction AniCommonConverter::Converter(const std::string &action)
{
    return ohos::update::UpgradeAction::from_value(action);
}

ohos::update::UpgradePolicy AniCommonConverter::Converter(const UpgradePolicy &upgradePolicy)
{
    std::vector<UpgradePeriod> vectors = { std::begin(upgradePolicy.autoUpgradePeriods),
        std::end(upgradePolicy.autoUpgradePeriods) };
    return {
        .downloadStrategy = upgradePolicy.downloadStrategy,
        .autoUpgradeStrategy = upgradePolicy.autoUpgradeStrategy,
        .autoUpgradePeriods = Converter(vectors)
    };
}

ohos::update::ComponentDescription AniCommonConverter::Converter(const ComponentDescription &componentDescription)
{
    return {
        .componentId = componentDescription.componentId,
        .descriptionInfo = Converter(componentDescription.descriptionInfo)
    };
}

taihe::array<ohos::update::ComponentDescription> AniCommonConverter::Converter(
    const std::vector<ComponentDescription> &componentDescriptions)
{
    std::vector<ohos::update::ComponentDescription> taiheComponents;
    taiheComponents.reserve(componentDescriptions.size());
    for (const auto &component : componentDescriptions) {
        taiheComponents.push_back(Converter(component));
    }
    return { taiheComponents };
}

ohos::update::VersionComponent AniCommonConverter::Converter(const VersionComponent &component)
{
    return {
        .componentId = component.componentId,
        .componentType = Converter(static_cast<ComponentType>(component.componentType)),
        .upgradeAction = Converter(component.upgradeAction),
        .displayVersion = component.displayVersion,
        .innerVersion = component.innerVersion,
        .size = static_cast<int32_t>(component.size),
        .effectiveMode = Converter(static_cast<EffectiveMode>(component.effectiveMode)),
        .descriptionInfo = Converter(component.descriptionInfo)
    };
}

taihe::array<ohos::update::VersionComponent> AniCommonConverter::Converter(
    const std::vector<VersionComponent> &components)
{
    std::vector<ohos::update::VersionComponent> taiheComponents;
    taiheComponents.reserve(components.size());
    for (const auto &component : components) {
        taiheComponents.push_back(Converter(component));
    }
    return { taiheComponents };
}

ohos::update::UpgradePeriod AniCommonConverter::Converter(const UpgradePeriod &upgradePeriod)
{
    return {
        .start = static_cast<int32_t>(upgradePeriod.start),
        .end = static_cast<int32_t>(upgradePeriod.end)
    };
}

taihe::array<ohos::update::UpgradePeriod> AniCommonConverter::Converter(
    const std::vector<UpgradePeriod> &upgradePeriods)
{
    std::vector<ohos::update::UpgradePeriod> taiheUpgradePeriods;
    taiheUpgradePeriods.reserve(upgradePeriods.size());
    for (const auto &period : upgradePeriods) {
        taiheUpgradePeriods.push_back(Converter(period));
    }
    return { taiheUpgradePeriods };
}

VersionDigestInfo AniCommonConverter::Converter(const ohos::update::VersionDigestInfo &versionDigestInfo)
{
    VersionDigestInfo res;
    res.versionDigest = versionDigestInfo.versionDigest;
    return res;
}

DescriptionOptions AniCommonConverter::Converter(const ohos::update::DescriptionOptions &descriptionOptions)
{
    DescriptionOptions res;
    res.format = static_cast<DescriptionFormat>(descriptionOptions.format.get_value());
    res.language = descriptionOptions.language;
    return res;
}

DownloadOptions AniCommonConverter::Converter(const ohos::update::DownloadOptions &downloadOptions)
{
    DownloadOptions res;
    res.allowNetwork = static_cast<NetType>(downloadOptions.allowNetwork.get_value());
    res.order = static_cast<Order>(downloadOptions.order.get_value());
    return res;
}

ResumeDownloadOptions AniCommonConverter::Converter(const ohos::update::ResumeDownloadOptions &resumeDownloadOptions)
{
    ResumeDownloadOptions res;
    res.allowNetwork = static_cast<NetType>(resumeDownloadOptions.allowNetwork.get_value());
    return res;
}

PauseDownloadOptions AniCommonConverter::Converter(const ohos::update::PauseDownloadOptions &pauseDownloadOptions)
{
    PauseDownloadOptions res;
    res.isAllowAutoResume = pauseDownloadOptions.isAllowAutoResume;
    return res;
}

UpgradeOptions AniCommonConverter::Converter(const ohos::update::UpgradeOptions &upgradeOptions)
{
    UpgradeOptions res;
    res.order = static_cast<Order>(upgradeOptions.order.get_value());
    return res;
}

ClearOptions AniCommonConverter::Converter(const ohos::update::ClearOptions &clearOptions)
{
    ClearOptions res;
    res.status = static_cast<UpgradeStatus>(clearOptions.status.get_value());
    return res;
}

UpgradeFile AniCommonConverter::Converter(const ohos::update::UpgradeFile &upgradeFile)
{
    UpgradeFile res;
    res.filePath = upgradeFile.filePath;
    res.fileType = static_cast<ComponentType>(upgradeFile.fileType.get_value());
    return res;
}

UpgradeInfo AniCommonConverter::Converter(const ohos::update::UpgradeInfo &info)
{
    UpgradeInfo upgradeInfo;
    upgradeInfo.upgradeApp = info.upgradeApp;
    upgradeInfo.businessType = Converter(info.businessType);
    return upgradeInfo;
}

BusinessType AniCommonConverter::Converter(const ohos::update::BusinessType &businessType)
{
    BusinessType business;
    business.vendor = businessType.vendor;
    business.subType = static_cast<BusinessSubType>(businessType.subType.get_value());
    return business;
}

UpgradePeriod AniCommonConverter::Converter(const ohos::update::UpgradePeriod &period)
{
    UpgradePeriod upgradePeriod;
    upgradePeriod.start = period.start;
    upgradePeriod.end = period.end;
    return upgradePeriod;
}

UpgradePolicy AniCommonConverter::Converter(const ohos::update::UpgradePolicy &policy)
{
    UpgradePolicy upgradePolicy;
    upgradePolicy.downloadStrategy = policy.downloadStrategy;
    upgradePolicy.autoUpgradeStrategy = policy.autoUpgradeStrategy;
    const std::size_t range = std::min(policy.autoUpgradePeriods.size(), std::size(upgradePolicy.autoUpgradePeriods));
    for (std::size_t index = 0; index < range; ++index) {
        upgradePolicy.autoUpgradePeriods[index] = Converter(policy.autoUpgradePeriods[index]);
    }
    return upgradePolicy;
}

ohos::update::EventId AniCommonConverter::Converter(const EventId &effectiveMode)
{
    return ohos::update::EventId::from_value(static_cast<int32_t>(effectiveMode));
}

ohos::update::EventInfo AniCommonConverter::Converter(const EventInfo &info)
{
    return {
        .eventId = Converter(info.eventId),
        .taskBody = Converter(info.taskBody)
    };
}

FactoryResetStrategy AniCommonConverter::Converter(const ohos::update::FactoryResetStrategy &factoryResetStrategy)
{
    FactoryResetStrategy resetStrategy;
    resetStrategy.scope = static_cast<FactoryResetScope>(factoryResetStrategy.scope.get_value());
    resetStrategy.strategy = factoryResetStrategy.strategy;
    return resetStrategy;
}

ohos::update::FactoryResetInfo AniCommonConverter::Converter(const FactoryResetInfo &factoryResetInfo)
{
    return {
        .duration = factoryResetInfo.duration
    };
}
}
