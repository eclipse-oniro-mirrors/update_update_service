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

#ifndef UPDATE_SERVICE_IMPL_FIRMWARE_H
#define UPDATE_SERVICE_IMPL_FIRMWARE_H

#include <condition_variable>

#include "iservice_online_updater.h"

#include "firmware_preferences_utils.h"
#include "firmware_task.h"

namespace OHOS {
namespace UpdateEngine {
class UpdateServiceImplFirmware final : public IServiceOnlineUpdater {
public:
    UpdateServiceImplFirmware() = default;

    ~UpdateServiceImplFirmware() = default;

    DISALLOW_COPY_AND_MOVE(UpdateServiceImplFirmware);

    int32_t CheckNewVersion(const UpgradeInfo &info, BusinessError &businessError, CheckResult &checkResult) final;

    int32_t Download(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
        const DownloadOptions &downloadOptions, BusinessError &businessError) final;

    int32_t PauseDownload(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
        const PauseDownloadOptions &pauseDownloadOptions, BusinessError &businessError) final;

    int32_t ResumeDownload(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
        const ResumeDownloadOptions &resumeDownloadOptions, BusinessError &businessError) final;

    int32_t Upgrade(const UpgradeInfo &info, const VersionDigestInfo &versionDigest,
        const UpgradeOptions &upgradeOptions, BusinessError &businessError) final;

    int32_t ClearError(const UpgradeInfo &info, const VersionDigestInfo &versionDigest,
        const ClearOptions &clearOptions, BusinessError &businessError) final;

    int32_t TerminateUpgrade(const UpgradeInfo &info, BusinessError &businessError) final;

    int32_t GetNewVersionInfo(
        const UpgradeInfo &info, NewVersionInfo &newVersionInfo, BusinessError &businessError) final;

    int32_t GetNewVersionDescription(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
        const DescriptionOptions &descriptionOptions, VersionDescriptionInfo &newVersionDescriptionInfo,
        BusinessError &businessError) final;

    int32_t GetCurrentVersionInfo(const UpgradeInfo &info, CurrentVersionInfo &currentVersionInfo,
        BusinessError &businessError) final;

    int32_t GetCurrentVersionDescription(const UpgradeInfo &info, const DescriptionOptions &descriptionOptions,
        VersionDescriptionInfo &currentVersionDescriptionInfo, BusinessError &businessError) final;

    int32_t GetTaskInfo(const UpgradeInfo &info, TaskInfo &taskInfo, BusinessError &businessError) final;

    int32_t SetUpgradePolicy(const UpgradeInfo &info, const UpgradePolicy &policy,
        BusinessError &businessError) final;

    int32_t GetUpgradePolicy(const UpgradeInfo &info, UpgradePolicy &policy, BusinessError &businessError) final;

    int32_t Cancel(const UpgradeInfo &info, int32_t service, BusinessError &businessError) final;

private:
    std::shared_ptr<FirmwarePreferencesUtil> preferencesUtil_ =
        DelayedSingleton<FirmwarePreferencesUtil>::GetInstance();
    std::condition_variable conditionVariable_;
    std::mutex checkNewVersionMutex_;
    bool checkComplete_ = false;
    void GetChangelogContent(std::string &dataXml, const std::string &language);
};
} // namespace UpdateEngine
} // namespace OHOS
#endif // UPDATE_SERVICE_IMPL_FIRMWARE_H