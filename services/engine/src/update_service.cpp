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
#include <updater_sa_ipc_interface_code.h>

#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "accesstoken_kit.h"
#include "access_manager.h"
#include "access_token.h"
#include "config_parse.h"
#include "dupdate_net_manager.h"
#include "firmware_common.h"
#include "firmware_manager.h"
#include "ipc_skeleton.h"
#include "module_manager.h"
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
constexpr const pid_t ROOT_UID = 0;
constexpr const pid_t EDM_UID = 3057;
REGISTER_SYSTEM_ABILITY_BY_ID(UpdateService, UPDATE_DISTRIBUTED_SERVICE_ID, true)

OHOS::sptr<UpdateService> UpdateService::updateService_ { nullptr };

#ifdef UPDATE_SERVICE_ENABLE_RUN_ON_DEMAND_QOS
constexpr int OPEN_SO_PRIO = -20;
constexpr int NORMAL_PRIO = 0;
#endif

void UpdateService::ClientDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    ENGINE_LOGI("client DeathRecipient OnRemoteDied: %{public}s", upgradeInfo_.ToString().c_str());
    sptr<UpdateService> service = UpdateService::GetInstance();
    int32_t funcResult = 0;
    if (service != nullptr) {
        service->UnregisterUpdateCallback(upgradeInfo_, funcResult);
    }
}

UpdateService::ClientProxy::ClientProxy(const UpgradeInfo &info, const sptr<IUpdateCallback> &callback)
    : proxy_(callback)
{
    ENGINE_LOGI("UpdateService::ClientProxy constructor");
    auto clientDeathRecipient = new (std::nothrow) ClientDeathRecipient(info);
    if (clientDeathRecipient != nullptr) {
        deathRecipient_ = sptr<ClientDeathRecipient>(clientDeathRecipient);
    } else {
        ENGINE_LOGE("UpdateService::ClientProxy, new fail");
    }
}

void UpdateService::ClientProxy::AddDeathRecipient()
{
    ENGINE_LOGI("UpdateService::ClientProxy AddDeathRecipient in");
    if (proxy_ != nullptr) {
        auto remoteObject = proxy_->AsObject();
        if ((remoteObject != nullptr) && (deathRecipient_ != nullptr)) {
            remoteObject->AddDeathRecipient(deathRecipient_);
            ENGINE_LOGI("UpdateService::ClientProxy AddDeathRecipient success");
        }
    }
}

void UpdateService::ClientProxy::RemoveDeathRecipient()
{
    ENGINE_LOGI("UpdateService::ClientProxy RemoveDeathRecipient in");
    if (proxy_ != nullptr) {
        auto remoteObject = proxy_->AsObject();
        if ((remoteObject != nullptr) && (deathRecipient_ != nullptr)) {
            remoteObject->RemoveDeathRecipient(deathRecipient_);
            ENGINE_LOGI("UpdateService::ClientProxy RemoveDeathRecipient success");
        }
    }
}

sptr<IUpdateCallback> UpdateService::ClientProxy::Get()
{
    return proxy_;
}

UpdateService::UpdateService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate)
{
    updateImplMgr_ = std::make_shared<UpdateServiceImplManager>();
}

UpdateService::~UpdateService()
{
    ENGINE_LOGI("UpdateServerTest free now");
    for (auto &iter : clientProxyMap_) {
        iter.second.RemoveDeathRecipient();
    }
}

sptr<UpdateService> UpdateService::GetInstance()
{
    return updateService_;
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

int32_t UpdateService::FactoryReset(BusinessError &businessError, int32_t &funcResult)
{
    sptr<UpdateServiceRestorer> restorer = sptr<UpdateServiceRestorer>::MakeSptr();
    if (restorer == nullptr) {
        ENGINE_LOGI("FactoryReset restorer null");
        return INT_CALL_FAIL;
    }
    return restorer->FactoryReset(businessError);
}

int32_t UpdateService::ForceFactoryReset(BusinessError &businessError, int32_t &funcResult)
{
    sptr<UpdateServiceRestorer> restorer = sptr<UpdateServiceRestorer>::MakeSptr();
    if (restorer == nullptr) {
        ENGINE_LOGI("ForceFactoryReset restorer null");
        return INT_CALL_FAIL;
    }
    return restorer->ForceFactoryReset(businessError);
}

int32_t UpdateService::DeepFactoryReset(const FactoryResetStrategy &factoryResetStrategy, BusinessError &businessError,
    int32_t &funcResult)
{
    sptr<UpdateServiceRestorer> restorer = sptr<UpdateServiceRestorer>::MakeSptr();
    if (restorer == nullptr) {
        ENGINE_LOGE("DeepFactoryReset restorer null");
        return INT_CALL_FAIL;
    }
    return restorer->DeepFactoryReset(factoryResetStrategy, businessError);
}

int32_t UpdateService::GetDeepFactoryResetInfo(const FactoryResetStrategy &factoryResetStrategy,
    FactoryResetInfo &factoryResetInfo, BusinessError &businessError, int32_t &funcResult)
{
    sptr<UpdateServiceRestorer> restorer = sptr<UpdateServiceRestorer>::MakeSptr();
    if (restorer == nullptr) {
        ENGINE_LOGE("DeepFactoryReset restorer null");
        return INT_CALL_FAIL;
    }
    return restorer->GetDeepFactoryResetInfo(factoryResetStrategy, factoryResetInfo, businessError);
}

int32_t UpdateService::ApplyNewVersion(const UpgradeInfo &info, const std::string &miscFile,
    const std::vector<std::string> &packageNames, BusinessError &businessError, int32_t &funcResult)
{
    sptr<UpdateServiceLocalUpdater> localUpdater = sptr<UpdateServiceLocalUpdater>::MakeSptr();
    if (localUpdater == nullptr) {
        ENGINE_LOGI("FactoryReset localUpdater null");
        return INT_CALL_FAIL;
    }
    return localUpdater->ApplyNewVersion(info, miscFile, packageNames, businessError);
}

int32_t UpdateService::VerifyUpgradePackage(const std::string &packagePath, const std::string &keyPath,
    BusinessError &businessError, int32_t &funcResult)
{
    sptr<UpdateServiceLocalUpdater> localUpdater = sptr<UpdateServiceLocalUpdater>::MakeSptr();
    if (localUpdater == nullptr) {
        ENGINE_LOGI("FactoryReset localUpdater null");
        return INT_CALL_FAIL;
    }
    return localUpdater->VerifyUpgradePackage(packagePath, keyPath, businessError);
}

void BuildUpgradeInfoDump(const int fd, UpgradeInfo &info)
{
    dprintf(fd, "---------------------upgrade info info--------------------\n");
    dprintf(fd, "UpgradeApp: %s\n", info.upgradeApp.c_str());
    dprintf(fd, "vendor: %s\n", info.businessType.vendor.c_str());
    dprintf(fd, "subType: %d\n", static_cast<int>(info.businessType.subType));
}

void BuildVersionInfoDump(const int fd, const CheckResult &checkResult)
{
    dprintf(fd, "---------------------version info--------------------\n");
    dprintf(fd, "isExistNewVersion: %d\n", checkResult.isExistNewVersion);
    if (checkResult.newVersionInfo.versionComponents.empty()) {
        return;
    }
    dprintf(fd, "PackageSize: %zu\n", static_cast<size_t>(checkResult.newVersionInfo.versionComponents[0].size));
    dprintf(fd, "ComponentType: %d\n", checkResult.newVersionInfo.versionComponents[0].componentType);
    dprintf(fd, "UpgradeAction: %s\n", checkResult.newVersionInfo.versionComponents[0].upgradeAction.c_str());
    dprintf(fd, "DisplayVersion: %s\n", checkResult.newVersionInfo.versionComponents[0].displayVersion.c_str());
    dprintf(fd, "InnerVersion: %s\n", checkResult.newVersionInfo.versionComponents[0].innerVersion.c_str());
    dprintf(fd, "Content: %s\n", checkResult.newVersionInfo.versionComponents[0].descriptionInfo.content.c_str());
}

void BuildTaskInfoDump(const int fd)
{
    sptr<UpdateService> service = UpdateService::GetInstance();
    if (service == nullptr) {
        ENGINE_LOGI("BuildTaskInfoDump no instance");
        return;
    }

    TaskInfo taskInfo;
    BusinessError businessError;
    UpgradeInfo upgradeInfo;
    int32_t funcResult = 0;
    service->GetTaskInfo(upgradeInfo, taskInfo, businessError, funcResult);
    if (!taskInfo.existTask) {
        dprintf(fd, "TaskInfo is empty\n");
        return;
    }

    dprintf(fd, "---------------------OTA status info--------------------\n");
    dprintf(fd, "Progress: %d\n", taskInfo.taskBody.progress);
    dprintf(fd, "UpgradeStatus: %d\n", taskInfo.taskBody.status);
    dprintf(fd, "SubStatus: %d\n", taskInfo.taskBody.subStatus);
    for (auto &iter : taskInfo.taskBody.errorMessages) {
        dprintf(fd, "ErrorCode: %d\n", iter.errorCode);
        dprintf(fd, "ErrorMsg: %s\n", iter.errorMessage.c_str());
    }
}

void UpdateService::DumpUpgradeCallback(const int fd)
{
    dprintf(fd, "---------------------callback info--------------------\n");
    std::lock_guard<std::mutex> lock(clientProxyMapLock_);
    for (const auto &iter : clientProxyMap_) {
        const UpgradeInfo& info = iter.first;
        dprintf(fd, "%s\n", info.ToString().c_str());
    }
}

int UpdateService::Dump(int fd, const std::vector<std::u16string> &args)
{
    if (!ModuleManager::GetInstance().IsModuleLoaded()) {
        if (fd < 0) {
            ENGINE_LOGI("HiDumper handle invalid");
            return -1;
        }

        if (args.size() == 0) {
            UpgradeInfo upgradeInfo = UpdateServiceCache::GetUpgradeInfo(BusinessSubType::FIRMWARE);
            BuildUpgradeInfoDump(fd, upgradeInfo);
            BuildTaskInfoDump(fd);
            DumpUpgradeCallback(fd);
        } else {
            dprintf(fd, "input error, no parameters required\n");
        }
        return 0;
    }

    return ModuleManager::GetInstance().HandleDumpFunc("Dump", fd, args);
}

#ifdef UPDATE_SERVICE_ENABLE_RUN_ON_DEMAND_QOS
void UpdateService::SetThreadPrio(int priority)
{
    int tid = syscall(SYS_gettid);
    ENGINE_LOGI("set tid: %{public}d priority:%{public}d.", tid, priority);
    if (setpriority(PRIO_PROCESS, tid, priority) != 0) {
        ENGINE_LOGE("set tid: %{public}d priority:%{public}d failed.", tid, priority);
    }
}
#endif

void UpdateService::OnStart(const SystemAbilityOnDemandReason &startReason)
{
    ENGINE_LOGI("UpdaterService oh OnStart, startReason name %{public}s, id %{public}d, value %{public}s",
        startReason.GetName().c_str(), CAST_INT(startReason.GetId()), startReason.GetValue().c_str());
    updateService_ = this;
    if (updateService_ == nullptr) {
        ENGINE_LOGE("updateService_ null");
    }

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
        ENGINE_LOGI("IsModuleLoaded false, init updateservice_sa");
        DelayedSingleton<NetManager>::GetInstance()->Init();

        // 动态启停流程启动
        DelayedSingleton<StartupManager>::GetInstance()->Start();
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

int32_t UpdateService::OnIdle(const SystemAbilityOnDemandReason &idleReason)
{
    ENGINE_LOGI("UpdaterService OnIdle");
    return ModuleManager::GetInstance().HandleOnIdleFunc("OnIdle", idleReason);
}

void UpdateService::OnStop(const SystemAbilityOnDemandReason &stopReason)
{
    ENGINE_LOGI("UpdaterService OnStop");
    ModuleManager::GetInstance().HandleOnStartOnStopFunc("OnStop", stopReason);
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

int32_t UpdateService::PermissionCheck(uint32_t code)
{
    ENGINE_LOGI("UpdateService Oh PermissionCheck, code: %{public}u", code);
    if (!IsCallerValid()) {
        ENGINE_LOGE("UpdateService IsCallerValid false");
        return INT_NOT_SYSTEM_APP;
    }

    if (!IsPermissionGranted(code)) {
        ENGINE_LOGE("UpdateService code %{public}u IsPermissionGranted false", code);
        return INT_APP_NOT_GRANTED;
    }

    if (code == CAST_UINT(UpdaterSaInterfaceCode::FACTORY_RESET)) {
        SYS_EVENT_SYSTEM_RESET(0, UpdateSystemEvent::RESET_START);
    }
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::CallbackExit(uint32_t code, int32_t result)
{
    return INT_CALL_SUCCESS;
}

int32_t UpdateService::CallbackParcel(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    // 未加载接口扩展能力，通过IDL框架分发
    if (!ModuleManager::GetInstance().IsModuleLoaded()) {
        ENGINE_LOGI("CallbackParcel, extend module not loaded, code %{public}u", code);
        return INT_CALL_SUCCESS;
    }

    // 加载了接口扩展能力， 但是当前调用接口不是拓展接口，还是通过idl框架分发
    if (!ModuleManager::GetInstance().IsMapFuncExist(code)) {
        ENGINE_LOGE("CallbackParcel, code %{public}u not extended", code);
        return INT_CALL_SUCCESS;
    }

    // 加载了接口扩展能力， 并且当前调用接口为拓展接口，则通过Hook框架分发
    int32_t ret = ModuleManager::GetInstance().HandleFunc(code, data, reply, option);
    ENGINE_LOGE("CallbackParcel, code %{public}u extended, result %{public}d", code, ret);
    if (ret != INT_CALL_SUCCESS) {
        return ret;
    }
    return INT_CALL_FAIL;
}

bool UpdateService::IsCallerValid()
{
    OHOS::Security::AccessToken::AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    auto callerTokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenType(callerToken);
    switch (callerTokenType) {
        case OHOS::Security::AccessToken::TypeATokenTypeEnum::TOKEN_HAP: {
            uint64_t callerFullTokenID = IPCSkeleton::GetCallingFullTokenID();
            // hap进程只允许系统应用调用
            return OHOS::Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(callerFullTokenID);
        }
        case OHOS::Security::AccessToken::TypeATokenTypeEnum::TOKEN_NATIVE: {
            pid_t callerUid = IPCSkeleton::GetCallingUid();
            // native进程只允许root权限和edm调用
            return callerUid == ROOT_UID || callerUid == EDM_UID;
        }
        default:
            // 其他情况调用予以禁止
            return false;
    }
}

bool UpdateService::IsPermissionGranted(uint32_t code)
{
    Security::AccessToken::AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    string permission = "ohos.permission.UPDATE_SYSTEM";
    if (code == CAST_UINT(UpdaterSaInterfaceCode::FACTORY_RESET) ||
        code == CAST_UINT(UpdaterSaInterfaceCode::DEEP_FACTORY_RESET) ||
        code == CAST_UINT(UpdaterSaInterfaceCode::GET_DEEP_FACTORY_RESET_INFO)) {
        permission = "ohos.permission.FACTORY_RESET";
    }
    if (code == CAST_UINT(UpdaterSaInterfaceCode::FORCE_FACTORY_RESET)) {
        permission = "ohos.permission.FORCE_FACTORY_RESET";
    }
    int verifyResult = Security::AccessToken::AccessTokenKit::VerifyAccessToken(callerToken, permission);
    bool isPermissionGranted = verifyResult == Security::AccessToken::PERMISSION_GRANTED;
    if (!isPermissionGranted) {
        ENGINE_LOGE("%{public}s not granted, code:%{public}u", permission.c_str(), code);
    }
    return isPermissionGranted;
}
} // namespace UpdateService
} // namespace OHOS
