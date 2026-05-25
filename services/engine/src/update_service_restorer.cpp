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

#include "update_service_restorer.h"

#include "access_token.h"
#include "accesstoken_kit.h"
#ifndef UPDATER_FUZZ
#include "fs_manager/mount.h"
#endif
#include "ipc_skeleton.h"
#include "string_utils.h"
#include "updaterkits/updaterkits.h"

#include "update_define.h"
#include "update_log.h"
#include "update_system_event.h"

namespace OHOS {
namespace UpdateService {
const std::string MISC_PATH = "/misc";
const std::string MISC_FILE = "/dev/block/by-name/misc";
const std::string CMD_WIPE_DATA = "--user_wipe_data";

static std::string GetCallingAppId()
{
    OHOS::Security::AccessToken::AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    auto callerTokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenType(callerToken);
    if (callerTokenType == OHOS::Security::AccessToken::TypeATokenTypeEnum::TOKEN_HAP) {
        Security::AccessToken::HapTokenInfo  hapTokenInfo;
        if (Security::AccessToken::AccessTokenKit::GetHapTokenInfo(callerToken, hapTokenInfo) != 0) {
            ENGINE_LOGE("Get hap token info error");
            return "";
        }
        return hapTokenInfo.bundleName;
    }
    if (callerTokenType == OHOS::Security::AccessToken::TypeATokenTypeEnum::TOKEN_NATIVE) {
        return std::to_string(IPCSkeleton::GetCallingUid());
    }
    return "";
}

static int32_t ExecReset(BusinessError &businessError, bool forceFlag)
{
    businessError.errorNum = CallResult::SUCCESS;
    auto miscBlockDev = Updater::GetBlockDeviceByMountPoint(MISC_PATH);
    ENGINE_LOGI("FactoryReset::misc path : %{public}s", miscBlockDev.c_str());
    ENGINE_CHECK(!miscBlockDev.empty(), miscBlockDev = MISC_FILE, "cannot get block device of partition");
    const std::string suffix = forceFlag ? "\n--reset_enter:forceFactoryReset|" : "\n--reset_enter:factoryReset|";
    const std::string paramData = CMD_WIPE_DATA + suffix + GetCallingAppId();
    int32_t ret = RebootAndCleanUserData(miscBlockDev, paramData) ? INT_CALL_SUCCESS : INT_CALL_FAIL;
    ENGINE_LOGI("FactoryReset result : %{public}d", ret);
    SYS_EVENT_SYSTEM_RESET(
        0, ret == INT_CALL_SUCCESS ? UpdateSystemEvent::EVENT_SUCCESS_RESULT : UpdateSystemEvent::EVENT_FAILED_RESULT);
    return ret;
}

static int32_t ExecuteDeepReset(BusinessError &businessError, const FactoryResetStrategy factoryResetStrategy)
{
    const std::string eraseType = (factoryResetStrategy.scope == FactoryResetScope::DATA) ? "DATA" : "DATA_AND_OS";
    bool result = RebootAndSecureErase(eraseType);
    ENGINE_LOGI("ExecuteDeepReset: %{public}s", StringUtils::GetBoolStr(result).c_str());
    return result ? INT_CALL_SUCCESS : INT_CALL_FAIL;
}

sptr<StorageManager::IStorageManager> UpdateServiceRestorer::GetStorageMgrProxy()
{
    auto samgr = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        ENGINE_LOGE("samgr empty error");
        return nullptr;
    }

    auto remote = samgr->GetSystemAbility(OHOS::STORAGE_MANAGER_MANAGER_ID);
    if (remote == nullptr) {
        ENGINE_LOGE("storage manager client samgr ability empty error");
        return nullptr;
    }

    auto storageMgrProxy = iface_cast<StorageManager::IStorageManager>(remote);
    if (storageMgrProxy == nullptr) {
        ENGINE_LOGE("storageMgrProxy empty error");
        return nullptr;
    }
    return storageMgrProxy;
}

int32_t UpdateServiceRestorer::FileManagerEraseKeys()
{
    sptr<StorageManager::IStorageManager> client = GetStorageMgrProxy();
    if (client == nullptr) {
        ENGINE_LOGE("get storage manager service failed");
        return INT_CALL_FAIL;
    }
    return client->EraseAllUserEncryptedKeys();
}

int32_t UpdateServiceRestorer::FactoryReset(BusinessError &businessError)
{
#ifndef UPDATER_UT
    return ExecReset(businessError, false);
#else
    return INT_CALL_SUCCESS;
#endif
}

int32_t UpdateServiceRestorer::ForceFactoryReset(BusinessError &businessError)
{
    // 删除文件类秘钥
    int32_t ret = FileManagerEraseKeys();
    if (ret != 0) {
        ENGINE_LOGE("file manager erase keys error");
        return INT_CALL_FAIL;
    }
    return ExecReset(businessError, true);
}

int32_t UpdateServiceRestorer::DeepFactoryReset(const FactoryResetStrategy factoryResetStrategy,
    BusinessError &businessError)
{
    ENGINE_LOGI("DeepFactoryReset, scope %{public}d, strategy %{public}s",
        CAST_INT(factoryResetStrategy.scope), factoryResetStrategy.strategy.c_str());
    businessError.errorNum = CallResult::SUCCESS;
    if (factoryResetStrategy.scope != FactoryResetScope::DATA &&
        factoryResetStrategy.scope !=FactoryResetScope::DATA_AND_OS) {
        businessError.errorNum = CallResult::FAIL;
        businessError.message = "strategy scope type error";
        return INT_CALL_FAIL;
    }
    return ExecuteDeepReset(businessError, factoryResetStrategy);
}

int32_t UpdateServiceRestorer::GetDeepFactoryResetInfo(const FactoryResetStrategy factoryResetStrategy,
    FactoryResetInfo &factoryResetInfo, BusinessError &businessError)
{
    ENGINE_LOGI("GetDeepFactoryResetInfo, scope %{public}d, strategy %{public}s",
        CAST_INT(factoryResetStrategy.scope), factoryResetStrategy.strategy.c_str());
    businessError.errorNum = CallResult::SUCCESS;
    if (factoryResetStrategy.scope != FactoryResetScope::DATA &&
        factoryResetStrategy.scope !=FactoryResetScope::DATA_AND_OS) {
        businessError.errorNum = CallResult::FAIL;
        businessError.message = "strategy scope type error";
        return INT_CALL_FAIL;
    }
    const std::string eraseType = (factoryResetStrategy.scope == FactoryResetScope::DATA) ? "DATA" : "DATA_AND_OS";
    uint32_t uint = EstimatedEraseTime(eraseType);
    factoryResetInfo.duration = static_cast<int>(uint);
    ENGINE_LOGI("duration %{public}d", factoryResetInfo.duration);
    return INT_CALL_SUCCESS;
}
} // namespace UpdateService
} // namespace OHOS
