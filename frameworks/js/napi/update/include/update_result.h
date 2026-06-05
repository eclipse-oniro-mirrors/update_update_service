/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef UPDATE_RESULT_H
#define UPDATE_RESULT_H

#include "check_result.h"
#include "current_version_info.h"
#include "factory_reset_info.h"
#include "napi_structs_base.h"
#include "new_version_info.h"
#include "progress.h"
#include "session_type.h"
#include "task_info.h"
#include "upgrade_policy.h"
#include "version_description_info.h"

namespace OHOS::UpdateService {
struct UpdateResult : NapiResult {
public:
    using BuildJSObject = std::function<int(napi_env env, napi_value &obj, const UpdateResult &napiResult)>;
    BuildJSObject buildJSObject;

    union ResultUnion {
        UpgradePolicy *upgradePolicy;
        Progress *progress;
        NewVersionInfo *newVersionInfo;
        VersionDescriptionInfo *versionDescriptionInfo;
        CheckResult *checkResult;
        CurrentVersionInfo *currentVersionInfo;
        TaskInfo *taskInfo;
        FactoryResetInfo *factoryResetInfo;
    };

    ResultUnion result;

    void Release()
    {
        ENGINE_LOGI("UpdateResult Release");
        if (type == SessionType::SESSION_DOWNLOAD || type == SessionType::SESSION_UPGRADE) {
            ReleaseValue<Progress>(result.progress);
        } else if (type == SessionType::SESSION_CHECK_VERSION) {
            ReleaseValue<CheckResult>(result.checkResult);
        } else if (type == SessionType::SESSION_GET_NEW_VERSION) {
            ReleaseValue<NewVersionInfo>(result.newVersionInfo);
        } else if (type == SessionType::SESSION_GET_NEW_VERSION_DESCRIPTION ||
            type == SessionType::SESSION_GET_CUR_VERSION_DESCRIPTION) {
            ReleaseValue<VersionDescriptionInfo>(result.versionDescriptionInfo);
        } else if (type == SessionType::SESSION_GET_CUR_VERSION) {
            ReleaseValue<CurrentVersionInfo>(result.currentVersionInfo);
        } else if (type == SessionType::SESSION_GET_POLICY) {
            ReleaseValue<UpgradePolicy>(result.upgradePolicy);
        } else if (type == SessionType::SESSION_GET_DEEP_FACTORY_RESET_INFO) {
            ReleaseValue<FactoryResetInfo>(result.factoryResetInfo);
        } else {
            ENGINE_LOGI("UpdateResult Release, unknow type");
        }
        ENGINE_LOGI("UpdateResult Release finish");
    }

    UpdateResult &operator = (const UpdateResult &updateResult)
    {
        if (&updateResult == this) {
            return *this;
        }
        buildJSObject = updateResult.buildJSObject;
        static_cast<NapiResult &>(*this) = updateResult;
        this->type = updateResult.type;
        this->businessError = updateResult.businessError;

        ENGINE_LOGI("UpdateResult operator type %{public}d", updateResult.type);
        if (type == SessionType::SESSION_DOWNLOAD || type == SessionType::SESSION_UPGRADE) {
            AssignValue<Progress>(updateResult.result.progress, result.progress);
        } else if (type == SessionType::SESSION_CHECK_VERSION) {
            AssignValue<CheckResult>(updateResult.result.checkResult, result.checkResult);
        } else if (type == SessionType::SESSION_GET_NEW_VERSION) {
            AssignValue<NewVersionInfo>(updateResult.result.newVersionInfo, result.newVersionInfo);
        } else if (type == SessionType::SESSION_GET_NEW_VERSION_DESCRIPTION ||
            type == SessionType::SESSION_GET_CUR_VERSION_DESCRIPTION) {
            AssignValue<VersionDescriptionInfo>(
                updateResult.result.versionDescriptionInfo, result.versionDescriptionInfo);
        } else if (type == SessionType::SESSION_GET_CUR_VERSION) {
            AssignValue<CurrentVersionInfo>(
                updateResult.result.currentVersionInfo, result.currentVersionInfo);
        } else if (type == SessionType::SESSION_GET_POLICY) {
            AssignValue<UpgradePolicy>(updateResult.result.upgradePolicy, result.upgradePolicy);
        } else if (type == SessionType::SESSION_GET_DEEP_FACTORY_RESET_INFO) {
            AssignValue<FactoryResetInfo>(
                updateResult.result.factoryResetInfo, result.factoryResetInfo);
        } else {
            ENGINE_LOGI("UpdateResult unknow type");
        }
        return *this;
    }
};
} // namespace OHOS::UpdateService
#endif // UPDATE_RESULT_H