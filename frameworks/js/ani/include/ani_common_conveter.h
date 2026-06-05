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

#ifndef ANI_COMMON_CONVETER_H
#define ANI_COMMON_CONVETER_H

#include "ohos.update.proj.hpp"

#include "check_result.h"
#include "clear_options.h"
#include "component_description.h"
#include "current_version_info.h"
#include "description_options.h"
#include "download_options.h"
#include "event_info.h"
#include "factory_reset_info.h"
#include "factory_reset_strategy.h"
#include "pause_download_options.h"
#include "resume_download_options.h"
#include "upgrade_status.h"
#include "task_body.h"
#include "task_info.h"
#include "upgrade_file.h"
#include "upgrade_info.h"
#include "upgrade_options.h"
#include "upgrade_policy.h"

namespace OHOS::UpdateService {
class AniCommonConverter {
public:
    static ohos::update::ErrorMessage Converter(const ErrorMessage &error);
    static taihe::array<ohos::update::ErrorMessage> Converter(const std::vector<ErrorMessage> &errors);
    static ohos::update::TaskInfo Converter(const TaskInfo &info);
    static ohos::update::CurrentVersionInfo Converter(const CurrentVersionInfo &info);
    static ohos::update::NewVersionInfo Converter(const NewVersionInfo &info);
    static ohos::update::CheckResult Converter(const CheckResult &result);
    static ohos::update::TaskBody Converter(const TaskBody &taskBody);
    static ohos::update::VersionDigestInfo Converter(const VersionDigestInfo &versionDigestInfo);
    static ohos::update::UpgradeStatus Converter(const UpgradeStatus &status);
    static ohos::update::ComponentType Converter(const ComponentType &componentType);
    static ohos::update::DescriptionType Converter(const DescriptionType &descriptionType);
    static ohos::update::EffectiveMode Converter(const EffectiveMode &effectiveMode);
    static ohos::update::EventId Converter(const EventId &effectiveMode);
    static ohos::update::DescriptionInfo Converter(const DescriptionInfo &description);
    static ohos::update::UpgradeAction Converter(const std::string &action);
    static ohos::update::UpgradePolicy Converter(const UpgradePolicy &upgradePolicy);
    static ohos::update::ComponentDescription Converter(const ComponentDescription &componentDescription);
    static taihe::array<ohos::update::ComponentDescription> Converter(
        const std::vector<ComponentDescription> &componentDescriptions);
    static ohos::update::VersionComponent Converter(const VersionComponent &component);
    static taihe::array<ohos::update::VersionComponent> Converter(const std::vector<VersionComponent> &components);
    static ohos::update::UpgradePeriod Converter(const UpgradePeriod &upgradePeriod);
    static taihe::array<ohos::update::UpgradePeriod> Converter(const std::vector<UpgradePeriod> &upgradePeriods);
    static ohos::update::EventInfo Converter(const EventInfo &info);
    static ohos::update::FactoryResetInfo Converter(const FactoryResetInfo &factoryResetInfo);
    static FactoryResetStrategy Converter(const ohos::update::FactoryResetStrategy &factoryResetStrategy);
    static VersionDigestInfo Converter(const ohos::update::VersionDigestInfo &versionDigestInfo);
    static DescriptionOptions Converter(const ohos::update::DescriptionOptions &descriptionOptions);
    static DownloadOptions Converter(const ohos::update::DownloadOptions &downloadOptions);
    static ResumeDownloadOptions Converter(const ohos::update::ResumeDownloadOptions &resumeDownloadOptions);
    static PauseDownloadOptions Converter(const ohos::update::PauseDownloadOptions &pauseDownloadOptions);
    static UpgradeOptions Converter(const ohos::update::UpgradeOptions &upgradeOptions);
    static ClearOptions Converter(const ohos::update::ClearOptions &clearOptions);
    static UpgradeFile Converter(const ohos::update::UpgradeFile &upgradeFile);
    static UpgradeInfo Converter(const ohos::update::UpgradeInfo &info);
    static BusinessType Converter(const ohos::update::BusinessType &businessType);
    static UpgradePeriod Converter(const ohos::update::UpgradePeriod &period);
    static UpgradePolicy Converter(const ohos::update::UpgradePolicy &policy);
};
}
#endif // ANI_COMMON_CONVETER_H
