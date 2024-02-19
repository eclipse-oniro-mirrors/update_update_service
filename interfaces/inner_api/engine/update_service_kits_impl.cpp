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

#include "update_service_kits_impl.h"

#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "iupdate_service.h"
#include "iupdate_callback.h"
#include "update_define.h"
#include "update_log.h"

namespace OHOS::UpdateEngine {
UpdateServiceKits &UpdateServiceKits::GetInstance()
{
    return DelayedRefSingleton<UpdateServiceKitsImpl>::GetInstance();
}

UpdateServiceKitsImpl::UpdateServiceKitsImpl() : BaseServiceKitsImpl<IUpdateService>(UPDATE_DISTRIBUTED_SERVICE_ID) {}

UpdateServiceKitsImpl::~UpdateServiceKitsImpl() = default;

int32_t UpdateServiceKitsImpl::RegisterUpdateCallback(const UpgradeInfo &info, const UpdateCallbackInfo &cb)
{
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);

    std::lock_guard<std::mutex> lock(remoteServerLock_);

    // 以下代码中sptr<IUpdateCallback>不能修改为auto,否则在重注册时有概率出现Crash
    sptr<IUpdateCallback> remoteUpdateCallback(new UpdateCallback(cb));
    ENGINE_CHECK(remoteUpdateCallback != nullptr, return INT_PARAM_ERR, "Failed to create remote callback");
    int32_t ret = updateService->RegisterUpdateCallback(info, remoteUpdateCallback);
    remoteUpdateCallbackMap_[info] = remoteUpdateCallback;
    ENGINE_LOGI("RegisterUpdateCallback %{public}s", ret == INT_CALL_SUCCESS ? "success" : "failure");
    return ret;
}

int32_t UpdateServiceKitsImpl::UnregisterUpdateCallback(const UpgradeInfo &info)
{
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);

    ENGINE_LOGI("UnregisterUpdateCallback");
    std::lock_guard<std::mutex> lock(remoteServerLock_);
    remoteUpdateCallbackMap_.erase(info);
    return updateService->UnregisterUpdateCallback(info);
}

int32_t UpdateServiceKitsImpl::CheckNewVersion(const UpgradeInfo &info, BusinessError &businessError,
    CheckResult &checkResult)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::CheckNewVersion");

    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->CheckNewVersion(info, businessError, checkResult);
}

int32_t UpdateServiceKitsImpl::Download(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const DownloadOptions &downloadOptions, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::Download");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->Download(info, versionDigestInfo, downloadOptions, businessError);
}

int32_t UpdateServiceKitsImpl::PauseDownload(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const PauseDownloadOptions &pauseDownloadOptions, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::PauseDownload");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->PauseDownload(info, versionDigestInfo, pauseDownloadOptions, businessError);
}

int32_t UpdateServiceKitsImpl::ResumeDownload(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const ResumeDownloadOptions &resumeDownloadOptions, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::ResumeDownload");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->ResumeDownload(info, versionDigestInfo, resumeDownloadOptions, businessError);
}

int32_t UpdateServiceKitsImpl::Upgrade(const UpgradeInfo &info, const VersionDigestInfo &versionDigest,
    const UpgradeOptions &upgradeOptions, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::Upgrade");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->Upgrade(info, versionDigest, upgradeOptions, businessError);
}

int32_t UpdateServiceKitsImpl::ClearError(const UpgradeInfo &info, const VersionDigestInfo &versionDigest,
    const ClearOptions &clearOptions, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::ClearError");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->ClearError(info, versionDigest, clearOptions, businessError);
}

int32_t UpdateServiceKitsImpl::TerminateUpgrade(const UpgradeInfo &info, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::TerminateUpgrade");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->TerminateUpgrade(info, businessError);
}

int32_t UpdateServiceKitsImpl::GetNewVersionInfo(const UpgradeInfo &info, NewVersionInfo &newVersionInfo,
    BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::GetNewVersionInfo");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->GetNewVersionInfo(info, newVersionInfo, businessError);
}

int32_t UpdateServiceKitsImpl::GetNewVersionDescription(const UpgradeInfo &info,
    const VersionDigestInfo &versionDigestInfo, const DescriptionOptions &descriptionOptions,
    VersionDescriptionInfo &newVersionDescriptionInfo, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::GetNewVersionDescription");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->GetNewVersionDescription(info, versionDigestInfo, descriptionOptions,
        newVersionDescriptionInfo, businessError);
}

int32_t UpdateServiceKitsImpl::GetCurrentVersionInfo(const UpgradeInfo &info, CurrentVersionInfo &currentVersionInfo,
    BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::GetCurrentVersionInfo");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->GetCurrentVersionInfo(info, currentVersionInfo, businessError);
}

int32_t UpdateServiceKitsImpl::GetCurrentVersionDescription(const UpgradeInfo &info,
    const DescriptionOptions &descriptionOptions, VersionDescriptionInfo &currentVersionDescriptionInfo,
    BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::GetCurrentVersionDescription");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->GetCurrentVersionDescription(info, descriptionOptions, currentVersionDescriptionInfo,
        businessError);
}

int32_t UpdateServiceKitsImpl::GetTaskInfo(const UpgradeInfo &info, TaskInfo &taskInfo, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::GetTaskInfo");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->GetTaskInfo(info, taskInfo, businessError);
}

int32_t UpdateServiceKitsImpl::SetUpgradePolicy(const UpgradeInfo &info, const UpgradePolicy &policy,
    BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::SetUpgradePolicy");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->SetUpgradePolicy(info, policy, businessError);
}

int32_t UpdateServiceKitsImpl::GetUpgradePolicy(const UpgradeInfo &info, UpgradePolicy &policy,
    BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::GetUpgradePolicy");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->GetUpgradePolicy(info, policy, businessError);
}

int32_t UpdateServiceKitsImpl::Cancel(const UpgradeInfo &info, int32_t service, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::Cancel %d", service);
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->Cancel(info, service, businessError);
}

int32_t UpdateServiceKitsImpl::FactoryReset(BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::FactoryReset");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->FactoryReset(businessError);
}

int32_t UpdateServiceKitsImpl::ApplyNewVersion(const UpgradeInfo &info, const std::string &miscFile,
    const std::string &packageName, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::ApplyNewVersion");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->ApplyNewVersion(info, miscFile, packageName, businessError);
}

int32_t UpdateServiceKitsImpl::VerifyUpgradePackage(const std::string &packagePath, const std::string &keyPath,
    BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceKitsImpl::VerifyUpgradePackage");
    auto updateService = GetService();
    RETURN_FAIL_WHEN_SERVICE_NULL(updateService);
    return updateService->VerifyUpgradePackage(packagePath, keyPath, businessError);
}

void UpdateServiceKitsImpl::RegisterCallback()
{
    ENGINE_LOGI("RegisterUpdateCallback size %{public}zu", remoteUpdateCallbackMap_.size());
    for (auto &iter : remoteUpdateCallbackMap_) {
        remoteServer_->RegisterUpdateCallback(iter.first, iter.second);
    }
}
} // namespace OHOS::UpdateEngine