/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "update_service_impl_firmware.h"

#include <string>

#include "firmware_check_data_processor.h"
#include "firmware_constant.h"
#include "firmware_component_operator.h"
#include "firmware_log.h"
#include "firmware_manager.h"
#include "firmware_status_cache.h"
#include "firmware_task_operator.h"
#include "device_adapter.h"
#include "firmware_update_helper.h"
#include "string_utils.h"
#include "update_service_cache.h"
#include "update_service_util.h"

namespace OHOS {
namespace UpdateEngine {
const std::string LANGUAGE_CHINESE = "<language name=\"zh-cn\" code=\"2052\">";
const std::string LANGUAGE_ENGLISH = "<language name=\"en-us\" code=\"1033\">";
const std::string LANGUAGE_END = "</language>";

int32_t UpdateServiceImplFirmware::CheckNewVersion(const UpgradeInfo &info, BusinessError &businessError,
    CheckResult &checkResult)
{
    wptr<UpdateServiceImplFirmware> weakPtr(this);
    FirmwareManager::GetInstance()->DoCheck(
        [&, weakPtr](const BusinessError &error, const CheckResult &result) {
            sptr<UpdateServiceImplFirmware> firmwareSptr = weakPtr.promote();
            if (firmwareSptr == nullptr) {
                FIRMWARE_LOGE("UpdateServiceImplFirmware has been destructed, CheckNewVersion is TimeOut");
                return;
            }
            businessError = error;
            checkResult = result;
            firmwareSptr->checkComplete_ = true;
            firmwareSptr->conditionVariable_.notify_all();
        });
    std::unique_lock<std::mutex> lock(checkNewVersionMutex_);
    constexpr int32_t waitTime = 40;
    conditionVariable_.wait_for(lock, std::chrono::seconds(waitTime), [&] { return checkComplete_; });
    if (!checkComplete_) {
        FIRMWARE_LOGE("CheckNewVersion is time out");
        businessError.errorNum = CallResult::TIME_OUT;
        businessError.message = "CheckNewVersion TimeOut";
    }
    weakPtr->checkComplete_ = false;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::Download(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const DownloadOptions &downloadOptions, BusinessError &businessError)
{
    FIRMWARE_LOGI("Download allowNetwork:%{public}d order:%{public}d", CAST_INT(downloadOptions.allowNetwork),
        CAST_INT(downloadOptions.order));
    //控制1秒内重复点击下载
    if (DelayedSingleton<FirmwareStatusCache>::GetInstance()->IsDownloadTriggered()) {
        FIRMWARE_LOGI("on downloading, not perrmit repeat submmit");
        businessError.Build(CallResult::FAIL, "repeat download error");
        return INT_CALL_SUCCESS;
    }

    FirmwareTask task;
    FirmwareTaskOperator firmwareTaskOperator;
    firmwareTaskOperator.QueryTask(task);
    if (task.status != UpgradeStatus::CHECK_VERSION_SUCCESS) {
        FIRMWARE_LOGI("download fail current status: %{public}d", CAST_INT(task.status));
        businessError.Build(CallResult::FAIL, "download error");
        return INT_CALL_SUCCESS;
    }

    firmwareTaskOperator.UpdateDownloadOptionByTaskId(task.taskId,
        DownloadMode::MANUAL, downloadOptions.allowNetwork, downloadOptions.order);
    DelayedSingleton<FirmwareManager>::GetInstance()->DoDownload(downloadOptions, businessError);
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::PauseDownload(const UpgradeInfo &info,
    const VersionDigestInfo &versionDigestInfo, const PauseDownloadOptions &pauseDownloadOptions,
    BusinessError &businessError)
{
    FIRMWARE_LOGI("PauseDownload isAllowAutoResume:%{public}s",
        StringUtils::GetBoolStr(pauseDownloadOptions.isAllowAutoResume).c_str());
    businessError.Build(CallResult::UN_SUPPORT, "pause download not support");
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::ResumeDownload(const UpgradeInfo &info,
    const VersionDigestInfo &versionDigestInfo, const ResumeDownloadOptions &resumeDownloadOptions,
    BusinessError &businessError)
{
    FIRMWARE_LOGI("ResumeDownload allowNetwork:%{public}d", CAST_INT(resumeDownloadOptions.allowNetwork));
    businessError.Build(CallResult::UN_SUPPORT, "resume download not support");
    return INT_CALL_SUCCESS;
}


int32_t UpdateServiceImplFirmware::Upgrade(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const UpgradeOptions &upgradeOptions, BusinessError &businessError)
{
    FIRMWARE_LOGI("Upgrade order = %{public}d", CAST_INT(upgradeOptions.order));
    FirmwareTask task;
    FirmwareTaskOperator firmwareTaskOperator;
    firmwareTaskOperator.QueryTask(task);
    firmwareTaskOperator.UpdateUpgradeModeByTaskId(task.taskId, UpgradeMode::MANUAL);
    DelayedSingleton<FirmwareManager>::GetInstance()->DoInstall(upgradeOptions, businessError,
        FirmwareUpdateHelper::GetInstallType());
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::ClearError(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const ClearOptions &clearOptions, BusinessError &businessError)
{
    FIRMWARE_LOGI("ClearError, versionDigestInfo %{public}s ClearOptions %{public}d",
        versionDigestInfo.versionDigest.c_str(),
        CAST_INT(clearOptions.status));
    DelayedSingleton<FirmwareManager>::GetInstance()->DoClearError(businessError);
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::TerminateUpgrade(const UpgradeInfo &info, BusinessError &businessError)
{
    FIRMWARE_LOGI("TerminateUpgrade");
    DelayedSingleton<FirmwareManager>::GetInstance()->DoTerminateUpgrade(businessError);
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::GetNewVersionInfo(
    const UpgradeInfo &info, NewVersionInfo &newVersionInfo, BusinessError &businessError)
{
    FIRMWARE_LOGI("GetNewVersionInfo");
    FirmwareTask task;
    FirmwareTaskOperator().QueryTask(task);
    std::vector<FirmwareComponent> components;
    FirmwareComponentOperator().QueryAll(components);
    FirmwareUpdateHelper::BuildNewVersionInfo(components, newVersionInfo.versionComponents);
    newVersionInfo.versionDigestInfo.versionDigest = task.taskId;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::GetNewVersionDescription(const UpgradeInfo &info,
    const VersionDigestInfo &versionDigestInfo, const DescriptionOptions &descriptionOptions,
    VersionDescriptionInfo &newVersionDescriptionInfo, BusinessError &businessError)
{
    FIRMWARE_LOGI("GetNewVersionDescription versionDigestInfo %{public}s format %{public}d language %{public}s",
        versionDigestInfo.versionDigest.c_str(),
        CAST_INT(descriptionOptions.format),
        descriptionOptions.language.c_str());

    businessError.Build(CallResult::SUCCESS, "start GetNewVersionDescription");
    std::vector<FirmwareComponent> components;
    FirmwareComponentOperator().QueryAll(components);
    if (components.size() == 0) {
        FIRMWARE_LOGI("GetNewVersionDescription: no data");
        businessError.Build(CallResult::FAIL, "GetNewVersionDescription failed");
        return INT_CALL_SUCCESS;
    }

    for (auto const &component : components) {
        ComponentDescription componentDescription;
        componentDescription.componentId = component.componentId;
        std::string changelogFilePath = Firmware::CHANGELOG_PATH + "/" + component.componentId + ".xml";
        if (!FileUtils::IsFileExist(changelogFilePath)) {
            FIRMWARE_LOGE("changelog file [%{public}s] is not exist!", changelogFilePath.c_str());
            businessError.Build(CallResult::FAIL, "GetNewVersionDescription failed");
            return INT_CALL_SUCCESS;
        }
        std::string dataXml = FileUtils::ReadDataFromFile(changelogFilePath);
        std::string dataXmlFinal = dataXml.substr(dataXml.find_first_of("|") + 1, dataXml.size());
        GetChangelogContent(dataXmlFinal, descriptionOptions.language);
        componentDescription.descriptionInfo.content = dataXmlFinal;
        componentDescription.descriptionInfo.descriptionType =
            static_cast<DescriptionType>(atoi(dataXml.substr(0, dataXml.find_first_of("|")).c_str()));
        newVersionDescriptionInfo.componentDescriptions.push_back(componentDescription);
    }
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::GetCurrentVersionInfo(const UpgradeInfo &info,
    CurrentVersionInfo &currentVersionInfo, BusinessError &businessError)
{
    FIRMWARE_LOGI("UpdateServiceImplFirmware::GetCurrentVersionInfo");
    businessError.errorNum = CallResult::SUCCESS;
    FirmwareUpdateHelper::BuildCurrentVersionInfo(currentVersionInfo);
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::GetCurrentVersionDescription(const UpgradeInfo &info,
    const DescriptionOptions &descriptionOptions, VersionDescriptionInfo &currentVersionDescriptionInfo,
    BusinessError &businessError)
{
    FIRMWARE_LOGI("GetCurrentVersionDescription format %{public}d language %{public}s",
        CAST_INT(descriptionOptions.format),
        descriptionOptions.language.c_str());

    ComponentDescription descriptionContent;
    descriptionContent.componentId =
        DelayedSingleton<FirmwarePreferencesUtil>::GetInstance()->ObtainString(Firmware::HOTA_CURRENT_COMPONENT_ID, "");
    if (descriptionContent.componentId.empty()) {
        businessError.Build(CallResult::FAIL, "GetCurrentVersionDescription failed");
        FIRMWARE_LOGE("componentId is null");
        return INT_CALL_SUCCESS;
    }

    std::string changelogFilePath = Firmware::CHANGELOG_PATH + "/" + descriptionContent.componentId + ".xml";
    if (!FileUtils::IsFileExist(changelogFilePath)) {
        FIRMWARE_LOGE("current changelog file [%{public}s] is not exist!", changelogFilePath.c_str());
        businessError.Build(CallResult::FAIL, "GetCurrentVersionDescription failed");
        return INT_CALL_SUCCESS;
    }
    std::string dataXml = FileUtils::ReadDataFromFile(changelogFilePath);
    std::string dataXmlFinal = dataXml.substr(dataXml.find_first_of("|") + 1, dataXml.size());
    GetChangelogContent(dataXmlFinal, descriptionOptions.language);
    descriptionContent.descriptionInfo.content = dataXmlFinal;
    descriptionContent.descriptionInfo.descriptionType =
        static_cast<DescriptionType>(atoi(dataXml.substr(0, dataXml.find_first_of("|")).c_str()));
    currentVersionDescriptionInfo.componentDescriptions.push_back(descriptionContent);
    businessError.Build(CallResult::SUCCESS, "GetCurrentVersionDescription ok");
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::GetTaskInfo(const UpgradeInfo &info, TaskInfo &taskInfo,
    BusinessError &businessError)
{
    FIRMWARE_LOGI("GetTaskInfo");
    businessError.errorNum = CallResult::SUCCESS;
    FirmwareTask task;
    FirmwareTaskOperator().QueryTask(task);
    if (task.isExistTask) {
        taskInfo.existTask = true;
        taskInfo.taskBody.status = static_cast<UpgradeStatus>(task.status);
        taskInfo.taskBody.progress = task.progress;
        taskInfo.taskBody.versionDigestInfo.versionDigest = task.taskId;
    }
    FIRMWARE_LOGI("GetTaskInfo existTask %{public}s status %{public}d , progress %{public}d",
        StringUtils::GetBoolStr(taskInfo.existTask).c_str(), CAST_INT(taskInfo.taskBody.status),
        taskInfo.taskBody.progress);
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::SetUpgradePolicy(const UpgradeInfo &info, const UpgradePolicy &policy,
    BusinessError &businessError)
{
    FIRMWARE_LOGI(
        "SetUpgradePolicy autoDownload %{public}d installmode %{public}d startTime %{public}d endTime %{public}d",
        policy.downloadStrategy, policy.autoUpgradeStrategy, policy.autoUpgradePeriods[0].start,
        policy.autoUpgradePeriods[1].end);
    businessError.errorNum = CallResult::SUCCESS;
    bool isAutoDownloadSwitchOn = preferencesUtil_->ObtainBool(Firmware::AUTO_DOWNLOAD_SWITCH, false);
    FIRMWARE_LOGI("SetUpgradePolicy isAutoDownloadSwitchOn %{public}s",
        StringUtils::GetBoolStr(isAutoDownloadSwitchOn).c_str());
    if (isAutoDownloadSwitchOn != policy.downloadStrategy) {
        DelayedSingleton<FirmwareManager>::GetInstance()->DoAutoDownloadSwitchChanged(policy.downloadStrategy);
    }
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::GetUpgradePolicy(const UpgradeInfo &info, UpgradePolicy &policy,
    BusinessError &businessError)
{
    FIRMWARE_LOGI("GetUpgradePolicy");
    bool isAutoDownloadSwitchOn = preferencesUtil_->ObtainBool(Firmware::AUTO_DOWNLOAD_SWITCH, false);
    FIRMWARE_LOGI("GetUpgradePolicy isAutoDownloadSwitchOn %{public}s",
        StringUtils::GetBoolStr(isAutoDownloadSwitchOn).c_str());
    policy.downloadStrategy = isAutoDownloadSwitchOn;
    policy.autoUpgradePeriods[0].start =
        static_cast<uint32_t>(Constant::ONE_HOUR_MINUTES * Firmware::NIGHT_UPGRADE_START_HOUR);
    policy.autoUpgradePeriods[0].end =
        static_cast<uint32_t>(Constant::ONE_HOUR_MINUTES * Firmware::NIGHT_UPGRADE_END_HOUR);
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceImplFirmware::Cancel(const UpgradeInfo &info, int32_t service, BusinessError &businessError)
{
    FIRMWARE_LOGI("Cancel %{public}d", service);
    businessError.errorNum = CallResult::SUCCESS;
    FirmwareTask task;
    FirmwareTaskOperator().QueryTask(task);
    if (task.status != UpgradeStatus::DOWNLOADING && task.status != UpgradeStatus::DOWNLOAD_PAUSE) {
        FIRMWARE_LOGI("Cancel fail current status: %{public}d", CAST_INT(task.status));
        businessError.Build(CallResult::FAIL, "Cancel download error");
    } else {
        DelayedSingleton<FirmwareManager>::GetInstance()->DoCancelDownload(businessError);
    }
    return INT_CALL_SUCCESS;
}

void UpdateServiceImplFirmware::GetChangelogContent(std::string &dataXml, const std::string &language)
{
    std::string languageStart = LANGUAGE_ENGLISH;
    if (language.compare("zh-cn") != 0) {
        languageStart = LANGUAGE_CHINESE;
    }
    StringUtils::StringRemove(dataXml, languageStart, LANGUAGE_END);
}
} // namespace UpdateEngine
} // namespace OHOS
