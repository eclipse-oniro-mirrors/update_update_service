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

#include <unistd.h>
#include <unordered_set>
#include <updater_sa_ipc_interface_code.h>

#include "system_ability_definition.h"

#include "audit_event_report.h"
#include "accesstoken_kit.h"
#include "access_token.h"
#include "ipc_skeleton.h"
#include "module_manager.h"
#include "parameters.h"
#include "tokenid_kit.h"
#include "update_log.h"
#include "update_service_cache.h"
#include "update_service_restorer.h"
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
constexpr const pid_t CDM_UID = 8011;
constexpr std::string_view MDM_DISABLE_RESET_PARA = "persist.edm.reset_factory_disallowed";
REGISTER_SYSTEM_ABILITY_BY_ID(UpdateService, UPDATE_DISTRIBUTED_SERVICE_ID, true)

OHOS::sptr<UpdateService> UpdateService::updateService_ { nullptr };

#ifdef UPDATE_SERVICE_ENABLE_RUN_ON_DEMAND_QOS
const int OPEN_SO_PRIO = -20;
const int NORMAL_PRIO = 0;
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

int32_t UpdateService::OnIdle(const SystemAbilityOnDemandReason &idleReason)
{
    ENGINE_LOGI("UpdaterService OnIdle");
    return ModuleManager::GetInstance().HandleOnIdleFunc("OnIdle", idleReason);
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

    if (IsResetDisabledByMdm(code)) {
        ENGINE_LOGE("UpdateService code %{public}u mdm not allow reset", code);
        int32_t ret = AuditEventReport::ReportResetAuditEvent(GetCallingAppId());
        ENGINE_LOGI("ReportResetAuditEvent result = %{public}d", ret);
        return INT_MDM_DISABLE_RESET;
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
            return callerUid == ROOT_UID || callerUid == EDM_UID || callerUid == CDM_UID;
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

bool UpdateService::IsResetDisabledByMdm(uint32_t code)
{
    const std::unordered_set<uint32_t> resetCodes = {
        CAST_UINT(UpdaterSaInterfaceCode::FORCE_FACTORY_RESET),
        CAST_UINT(UpdaterSaInterfaceCode::FACTORY_RESET),
        CAST_UINT(UpdaterSaInterfaceCode::DEEP_FACTORY_RESET)
    };
    if (resetCodes.count(code)) {
        return OHOS::system::GetBoolParameter(MDM_DISABLE_RESET_PARA.data(), false);
    }
    return false;
}

std::string UpdateService::GetCallingAppId()
{
    OHOS::Security::AccessToken::AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    auto callerTokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenType(callerToken);
    if (callerTokenType == OHOS::Security::AccessToken::TypeATokenTypeEnum::TOKEN_HAP) {
        Security::AccessToken::HapTokenInfo  hapTokenInfo;
        int32_t ret = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(callerToken, hapTokenInfo);
        if (ret != 0) {
            ENGINE_LOGE("Get hap token info error, ret %{public}d", ret);
            return "";
        }
        return hapTokenInfo.bundleName;
    }
    if (callerTokenType == OHOS::Security::AccessToken::TypeATokenTypeEnum::TOKEN_NATIVE) {
        return std::to_string(IPCSkeleton::GetCallingUid());
    }
    return "";
}
} // namespace UpdateService
} // namespace OHOS