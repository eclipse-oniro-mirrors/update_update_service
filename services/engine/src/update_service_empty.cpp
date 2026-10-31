/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "update_service.h"

#include "config_parse.h"
#include "module_manager.h"
#include "update_log.h"

using namespace std;

namespace OHOS {
namespace UpdateService {
UpdateService::UpdateService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate) {}

int32_t UpdateService::RegisterUpdateCallback(const UpgradeInfo &info, const sptr<IUpdateCallback> &updateCallback,
    int32_t &funcResult)
{
    ENGINE_LOGI("RegisterUpdateCallback not support");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::UnregisterUpdateCallback(const UpgradeInfo &info, int32_t &funcResult)
{
    ENGINE_LOGI("UnregisterUpdateCallback not support");
    return INT_CALL_SUCCESS;
}

sptr<IUpdateCallback> UpdateService::GetUpgradeCallback(const UpgradeInfo &info, int32_t &funcResult)
{
    ENGINE_LOGI("GetUpgradeCallback not support");
    return nullptr;
}

int32_t UpdateService::GetNewVersionInfo(const UpgradeInfo &info, NewVersionInfo &newVersionInfo,
    BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("GetNewVersionInfo not support");
    businessError.Build(CallResult::UN_SUPPORT, "GetNewVersionInfo unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::GetNewVersionDescription(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const DescriptionOptions &descriptionOptions, VersionDescriptionInfo &newVersionDescriptionInfo,
    BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("GetNewVersionDescription not support");
    businessError.Build(CallResult::UN_SUPPORT, "GetNewVersionDescription unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::GetCurrentVersionInfo(const UpgradeInfo &info, CurrentVersionInfo &currentVersionInfo,
    BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("GetCurrentVersionInfo not support");
    businessError.Build(CallResult::UN_SUPPORT, "GetCurrentVersionInfo unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::GetCurrentVersionDescription(const UpgradeInfo &info,
    const DescriptionOptions &descriptionOptions, VersionDescriptionInfo &currentVersionDescriptionInfo,
    BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("GetCurrentVersionDescription not support");
    businessError.Build(CallResult::UN_SUPPORT, "GetCurrentVersionDescription unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::GetTaskInfo(const UpgradeInfo &info, TaskInfo &taskInfo, BusinessError &businessError,
    int32_t &funcResult)
{
    ENGINE_LOGI("GetTaskInfo not support");
    businessError.Build(CallResult::UN_SUPPORT, "GetTaskInfo unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::SetUpgradePolicy(const UpgradeInfo &info, const UpgradePolicy &policy,
    BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("SetUpgradePolicy not support");
    businessError.Build(CallResult::UN_SUPPORT, "SetUpgradePolicy unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::GetUpgradePolicy(const UpgradeInfo &info, UpgradePolicy &policy, BusinessError &businessError,
    int32_t &funcResult)
{
    ENGINE_LOGI("GetUpgradePolicy not support");
    businessError.Build(CallResult::UN_SUPPORT, "GetUpgradePolicy unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::CheckNewVersion(const UpgradeInfo &info, BusinessError &businessError, CheckResult &checkResult,
    int32_t &funcResult)
{
    ENGINE_LOGI("CheckNewVersion not support");
    businessError.Build(CallResult::UN_SUPPORT, "CheckNewVersion unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::Download(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const DownloadOptions &downloadOptions, BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("Download not support");
    businessError.Build(CallResult::UN_SUPPORT, "Download unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::PauseDownload(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const PauseDownloadOptions &pauseDownloadOptions, BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("PauseDownload not support");
    businessError.Build(CallResult::UN_SUPPORT, "PauseDownload unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::ResumeDownload(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const ResumeDownloadOptions &resumeDownloadOptions, BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("ResumeDownload not support");
    businessError.Build(CallResult::UN_SUPPORT, "ResumeDownload unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::Upgrade(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const UpgradeOptions &upgradeOptions, BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("Upgrade not support");
    businessError.Build(CallResult::UN_SUPPORT, "Upgrade unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::ClearError(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const ClearOptions &clearOptions, BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("ClearError not support");
    businessError.Build(CallResult::UN_SUPPORT, "ClearError unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::TerminateUpgrade(const UpgradeInfo &info, BusinessError &businessError,
    int32_t &funcResult)
{
    ENGINE_LOGI("TerminateUpgrade not support");
    businessError.Build(CallResult::UN_SUPPORT, "TerminateUpgrade unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::Cancel(const UpgradeInfo &info, int32_t service, BusinessError &businessError,
    int32_t &funcResult)
{
    ENGINE_LOGI("Cancel not support");
    businessError.Build(CallResult::UN_SUPPORT, "Cancel unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::ApplyNewVersion(const UpgradeInfo &info, const std::string &miscFile,
    const std::vector<std::string> &packageNames, BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("ApplyNewVersion not support");
    businessError.Build(CallResult::UN_SUPPORT, "ApplyNewVersion unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::VerifyUpgradePackage(const std::string &packagePath, const std::string &keyPath,
    BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("VerifyUpgradePackage not support");
    businessError.Build(CallResult::UN_SUPPORT, "VerifyUpgradePackage unsupport");
    return INT_CALL_SUCCESS;
}

void UpdateService::OnStart(const SystemAbilityOnDemandReason &startReason)
{
    ENGINE_LOGI("UpdaterService oh OnStart, startReason name %{public}s, id %{public}d, value %{public}s",
        startReason.GetName().c_str(), CAST_INT(startReason.GetId()), startReason.GetValue().c_str());
    updateService_ = this;

    DelayedSingleton<ConfigParse>::GetInstance()->LoadConfigInfo(); // 启动读取配置信息
    std::string libPath = DelayedSingleton<ConfigParse>::GetInstance()->GetModuleLibPath();
    ENGINE_LOGI("GetModuleLibPath %{public}s ", libPath.c_str());
#ifdef UPDATE_SERVICE_ENABLE_RUN_ON_DEMAND_QOS
    constexpr int OPEN_SO_PRIO = -20;
    SetThreadPrio(OPEN_SO_PRIO);
#endif
    ModuleManager::GetInstance().LoadModule(libPath);
#ifdef UPDATE_SERVICE_ENABLE_RUN_ON_DEMAND_QOS
    constexpr int NORMAL_PRIO = 0;
    SetThreadPrio(NORMAL_PRIO);
#endif
    if (Publish(this)) {
        ENGINE_LOGI("UpdaterService OnStart publish success");
    } else {
        ENGINE_LOGI("UpdaterService OnStart publish fail");
    }

    if (ModuleManager::GetInstance().IsModuleLoaded()) {
        ENGINE_LOGI("IsModuleLoaded true");
        ModuleManager::GetInstance().HandleOnStartOnStopFunc("OnStart", startReason);
    }
}

void UpdateService::OnStop(const SystemAbilityOnDemandReason &stopReason)
{
    ENGINE_LOGI("UpdaterService OnStop begin");
    ModuleManager::GetInstance().HandleOnStartOnStopFunc("OnStop", stopReason);
    ENGINE_LOGI("UpdaterService OnStop end");
}

int32_t UpdateService::CallbackEnter(uint32_t code)
{
    // 未加载接口扩展能力，执行UpdateService鉴权动作
    if (!ModuleManager::GetInstance().IsModuleLoaded()) {
        ENGINE_LOGI("CallbackEnter, extend module not loaded, code %{public}u", code);
        return PermissionCheck(code);
    }

    // 加载了接口扩展能力， 但是当前调用接口不是拓展接口，还是执行UpdateService鉴权动作
    if (!ModuleManager::GetInstance().IsMapFuncExist(code)) {
        ENGINE_LOGE("CallbackEnter, code %{public}u not extended", code);
        return PermissionCheck(code);
    }

    // 加载了接口扩展能力，并且当前调用的接口为拓展接口，则由Hook框架进行接口鉴权
    return INT_CALL_SUCCESS;
}
} // namespace UpdateService
} // namespace OHOS
