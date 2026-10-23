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

#include <dlfcn.h>
#include <unistd.h>
#include <unordered_set>
#include <updater_sa_ipc_interface_code.h>

#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "audit_event_report.h"
#include "accesstoken_kit.h"
#include "access_manager.h"
#include "access_token.h"
#include "config_parse.h"
#include "dupdate_net_manager.h"
#include "firmware_common.h"
#include "ipc_skeleton.h"
#include "module_manager.h"
#include "parameter.h"
#include "parameters.h"
#include "securec.h"
#include "startup_manager.h"
#include "tokenid_kit.h"
#include "update_log.h"
#include "update_service_cache.h"
#include "update_service_local_updater.h"
#include "update_service_module.h"
#include "update_service_restorer.h"
#include "update_service_util.h"
#include "update_system_event.h"

#ifdef UPDATE_SERVICE_ENABLE_RUN_ON_DEMAND_QOS
#include <sys/syscall.h>
#include <sys/resource.h>
#endif

using namespace std;

namespace OHOS {
namespace UpdateService {
#ifdef UPDATE_SERVICE_ENABLE_RUN_ON_DEMAND_QOS
constexpr int OPEN_SO_PRIO = -20;
constexpr int NORMAL_PRIO = 0;
#endif

UpdateService::UpdateService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate)
{
    updateImplMgr_ = std::make_shared<UpdateServiceImplManager>();
}

int32_t UpdateService::RegisterUpdateCallback(const UpgradeInfo &info, const sptr<IUpdateCallback> &updateCallback,
    int32_t &funcResult)
{
    ENGINE_LOGI("RegisterUpdateCallback");
    UnregisterUpdateCallback(info, funcResult);
    {
        std::lock_guard<std::mutex> lock(clientProxyMapLock_);
        ClientProxy clientProxy(info, updateCallback);
        clientProxy.AddDeathRecipient();
        clientProxyMap_.insert({info, clientProxy});
        DelayedSingleton<AccessManager>::GetInstance()->SetRemoteIdle(clientProxyMap_.empty());
    }
    if (!info.IsLocal()) {
        UpdateServiceCache::SetUpgradeInfo(info);
    }
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::UnregisterUpdateCallback(const UpgradeInfo &info, int32_t &funcResult)
{
    ENGINE_LOGI("UnregisterUpdateCallback");
    std::lock_guard<std::mutex> lock(clientProxyMapLock_);
    auto iter = clientProxyMap_.find(info);
    if (iter == clientProxyMap_.end()) {
        return INT_CALL_SUCCESS;
    }
    iter->second.RemoveDeathRecipient();
    clientProxyMap_.erase(info);
    DelayedSingleton<AccessManager>::GetInstance()->SetRemoteIdle(clientProxyMap_.empty());
    return INT_CALL_SUCCESS;
}

sptr<IUpdateCallback> UpdateService::GetUpgradeCallback(const UpgradeInfo &info, int32_t &funcResult)
{
    std::lock_guard<std::mutex> lock(clientProxyMapLock_);
    auto iter = clientProxyMap_.find(info);
    if (iter == clientProxyMap_.end()) {
        return nullptr;
    }
    return iter->second.Get();
}

int32_t UpdateService::GetNewVersionInfo(const UpgradeInfo &info, NewVersionInfo &newVersionInfo,
    BusinessError &businessError, int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("GetNewVersionInfo onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->GetNewVersionInfo(info, newVersionInfo, businessError);
}

int32_t UpdateService::GetNewVersionDescription(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const DescriptionOptions &descriptionOptions, VersionDescriptionInfo &newVersionDescriptionInfo,
    BusinessError &businessError, int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("GetNewVersionDescription onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->GetNewVersionDescription(info, versionDigestInfo, descriptionOptions,
        newVersionDescriptionInfo, businessError);
}

int32_t UpdateService::GetCurrentVersionInfo(const UpgradeInfo &info, CurrentVersionInfo &currentVersionInfo,
    BusinessError &businessError, int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("GetCurrentVersionInfo onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->GetCurrentVersionInfo(info, currentVersionInfo, businessError);
}

int32_t UpdateService::GetCurrentVersionDescription(const UpgradeInfo &info,
    const DescriptionOptions &descriptionOptions, VersionDescriptionInfo &currentVersionDescriptionInfo,
    BusinessError &businessError, int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("GetCurrentVersionDescription onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->GetCurrentVersionDescription(info, descriptionOptions, currentVersionDescriptionInfo,
        businessError);
}

int32_t UpdateService::GetTaskInfo(const UpgradeInfo &info, TaskInfo &taskInfo, BusinessError &businessError,
    int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("GetTaskInfo onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->GetTaskInfo(info, taskInfo, businessError);
}

int32_t UpdateService::SetUpgradePolicy(const UpgradeInfo &info, const UpgradePolicy &policy,
    BusinessError &businessError, int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("SetUpgradePolicy onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->SetUpgradePolicy(info, policy, businessError);
}

int32_t UpdateService::GetUpgradePolicy(const UpgradeInfo &info, UpgradePolicy &policy, BusinessError &businessError,
    int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("GetUpgradePolicy onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->GetUpgradePolicy(info, policy, businessError);
}

int32_t UpdateService::CheckNewVersion(const UpgradeInfo &info, BusinessError &businessError, CheckResult &checkResult,
    int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("CheckNewVersion onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->CheckNewVersion(info, businessError, checkResult);
}

int32_t UpdateService::Download(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const DownloadOptions &downloadOptions, BusinessError &businessError, int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("Download onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->Download(info, versionDigestInfo, downloadOptions, businessError);
}

int32_t UpdateService::PauseDownload(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const PauseDownloadOptions &pauseDownloadOptions, BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("PauseDownload");
    businessError.errorNum = CallResult::SUCCESS;
    businessError.Build(CallResult::UN_SUPPORT, "PauseDownload unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::ResumeDownload(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const ResumeDownloadOptions &resumeDownloadOptions, BusinessError &businessError, int32_t &funcResult)
{
    ENGINE_LOGI("ResumeDownload allowNetwork:%{public}d", CAST_INT(resumeDownloadOptions.allowNetwork));
    businessError.Build(CallResult::UN_SUPPORT, "ResumeDownload unsupport");
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::Upgrade(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const UpgradeOptions &upgradeOptions, BusinessError &businessError, int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("Upgrade onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->Upgrade(info, versionDigestInfo, upgradeOptions, businessError);
}

int32_t UpdateService::ClearError(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const ClearOptions &clearOptions, BusinessError &businessError, int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("ClearError onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->ClearError(info, versionDigestInfo, clearOptions, businessError);
}

int32_t UpdateService::TerminateUpgrade(const UpgradeInfo &info, BusinessError &businessError,
    int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("TerminateUpgrade onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->TerminateUpgrade(info, businessError);
}

int32_t UpdateService::Cancel(const UpgradeInfo &info, int32_t service, BusinessError &businessError,
    int32_t &funcResult)
{
    sptr<IServiceOnlineUpdater> onlineUpdater = updateImplMgr_->GetOnlineUpdater(info);
    if (onlineUpdater == nullptr) {
        ENGINE_LOGI("Cancel onlineUpdater null");
        return INT_CALL_FAIL;
    }
    return onlineUpdater->Cancel(info, service, businessError);
}

int32_t UpdateService::ApplyNewVersion(const UpgradeInfo &info, const std::string &miscFile,
    const std::vector<std::string> &packageNames, BusinessError &businessError, int32_t &funcResult)
{
    sptr<UpdateServiceLocalUpdater> localUpdater = sptr<UpdateServiceLocalUpdater>::MakeSptr();
    if (localUpdater == nullptr) {
        ENGINE_LOGI("ApplyNewVersion localUpdater null");
        return INT_CALL_FAIL;
    }
    return localUpdater->ApplyNewVersion(info, miscFile, packageNames, businessError);
}

int32_t UpdateService::VerifyUpgradePackage(const std::string &packagePath, const std::string &keyPath,
    BusinessError &businessError, int32_t &funcResult)
{
    sptr<UpdateServiceLocalUpdater> localUpdater = sptr<UpdateServiceLocalUpdater>::MakeSptr();
    if (localUpdater == nullptr) {
        ENGINE_LOGI("VerifyUpgradePackage localUpdater null");
        return INT_CALL_FAIL;
    }
    return localUpdater->VerifyUpgradePackage(packagePath, keyPath, businessError);
}

void UpdateService::InitBusinessAfterUnlock()
{
    ENGINE_LOGI("Initialize NetManager after user unlock");
    DelayedSingleton<NetManager>::GetInstance()->Init();
    ENGINE_LOGI("Start StartupManager after user unlock");
    DelayedSingleton<StartupManager>::GetInstance()->Start();
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
    SetThreadPrio(OPEN_SO_PRIO);
#endif
    ModuleManager::GetInstance().LoadModule(libPath);
#ifdef UPDATE_SERVICE_ENABLE_RUN_ON_DEMAND_QOS
    SetThreadPrio(NORMAL_PRIO);
#endif

    if (!ModuleManager::GetInstance().IsModuleLoaded()) {
        if (userUnlockObserver_ == nullptr) {
            userUnlockObserver_ = std::make_shared<UpdateUserUnlockObserver>(
                [this]() {
                    InitBusinessAfterUnlock();
                });
        }
        userUnlockObserver_->Start();
    }

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
    if (userUnlockObserver_ != nullptr) {
        userUnlockObserver_->Stop();
    }
    ModuleManager::GetInstance().HandleOnStartOnStopFunc("OnStop", stopReason);
    ENGINE_LOGI("UpdaterService OnStop end");
}

bool UpdateService::IsUserUnlockRequired(uint32_t code)
{
    static const std::unordered_set<uint32_t> exemptCodes {
        static_cast<uint32_t>(UpdaterSaInterfaceCode::FACTORY_RESET),
        static_cast<uint32_t>(UpdaterSaInterfaceCode::APPLY_NEW_VERSION),
        static_cast<uint32_t>(UpdaterSaInterfaceCode::VERIFY_UPGRADE_PACKAGE),
        static_cast<uint32_t>(UpdaterSaInterfaceCode::FORCE_FACTORY_RESET),
        static_cast<uint32_t>(UpdaterSaInterfaceCode::DEEP_FACTORY_RESET),
        static_cast<uint32_t>(UpdaterSaInterfaceCode::GET_DEEP_FACTORY_RESET_INFO),
    };
    return exemptCodes.find(code) == exemptCodes.end();
}

int32_t UpdateService::CallbackEnter(uint32_t code)
{
    // 未加载接口扩展能力，执行UpdateService鉴权动作
    if (!ModuleManager::GetInstance().IsModuleLoaded()) {
        if (IsUserUnlockRequired(code) &&
            (userUnlockObserver_ == nullptr || !userUnlockObserver_->HasUserUnlocked())) {
            ENGINE_LOGE("CallbackEnter forbidden, active user is not unlocked, code %{public}u", code);
            return INT_FORBIDDEN;
        }
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
