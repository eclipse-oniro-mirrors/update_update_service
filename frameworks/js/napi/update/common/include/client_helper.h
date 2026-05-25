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

#ifndef UPDATE_CLIENT_HELPER_H
#define UPDATE_CLIENT_HELPER_H

#include <string>

#include "node_api.h"

#include "clear_options.h"
#include "description_format.h"
#include "description_options.h"
#include "download_options.h"
#include "event_classify_info.h"
#include "event_info.h"
#include "factory_reset_strategy.h"
#include "napi_common_utils.h"
#include "network_type.h"
#include "order.h"
#include "pause_download_options.h"
#include "resume_download_options.h"
#include "update_result.h"
#include "upgrade_file.h"
#include "upgrade_info.h"
#include "upgrade_options.h"
#include "upgrade_policy.h"

namespace OHOS::UpdateService {
class ClientHelper {
public:
    static int32_t BuildCheckResult(napi_env env, napi_value &obj, const UpdateResult &result);
    static int32_t BuildCurrentVersionInfo(napi_env env, napi_value &obj, const UpdateResult &result);
    static ClientStatus BuildEventInfo(napi_env env, napi_value &obj, const EventInfo &eventInfo);
    static int32_t BuildNewVersionInfo(napi_env env, napi_value &obj, const UpdateResult &result);
    static int32_t BuildVersionDescriptionInfo(napi_env env, napi_value &obj, const UpdateResult &result);
    static int32_t BuildUpgradePolicy(napi_env env, napi_value &obj, const UpdateResult &result);
    static int32_t BuildTaskInfo(napi_env env, napi_value &obj, const UpdateResult &result);
    static int32_t BuildUndefinedStatus(napi_env env, napi_value &obj, const UpdateResult &result);
    static int32_t BuildFactoryResetInfo(napi_env env, napi_value &obj, const UpdateResult &result);

    static ClientStatus GetUpgradeInfoFromArg(napi_env env, const napi_value arg, UpgradeInfo &upgradeInfo);
    static ClientStatus GetUpgradePolicyFromArg(napi_env env, const napi_value arg, UpgradePolicy &upgradePolicy);
    static ClientStatus GetUpgradeFileFromArg(napi_env env, const napi_value arg, UpgradeFile &upgradeFile);
    static ClientStatus GetUpgradeFilesFromArg(napi_env env, const napi_value arg,
        std::vector<UpgradeFile> &upgradeFiles);
    static ClientStatus GetVersionDigestInfoFromArg(napi_env env, const napi_value arg,
        VersionDigestInfo &versionDigestInfo);
    static ClientStatus GetOptionsFromArg(napi_env env, const napi_value arg, DescriptionOptions &descriptionOptions);
    static ClientStatus GetOptionsFromArg(napi_env env, const napi_value arg, UpgradeOptions &upgradeOptions);
    static ClientStatus GetOptionsFromArg(napi_env env, const napi_value arg, ClearOptions &clearOptions);
    static ClientStatus GetOptionsFromArg(napi_env env, const napi_value arg, DownloadOptions &downloadOptions);
    static ClientStatus GetOptionsFromArg(napi_env env, const napi_value arg,
        PauseDownloadOptions &pauseDownloadOptions);
    static ClientStatus GetOptionsFromArg(napi_env env, const napi_value arg,
        ResumeDownloadOptions &resumeDownloadOptions);
    static ClientStatus GetEventClassifyInfoFromArg(napi_env env, const napi_value arg,
        EventClassifyInfo &eventClassifyInfo);
    static ClientStatus GetFactoryResetStrategyFromArg(napi_env env, const napi_value arg,
        FactoryResetStrategy &factoryResetStrategy);

    static bool IsValidUpgradeFile(const std::string &upgradeFile);

    static void TrimString(std::string &str);

private:
    static ClientStatus GetDescriptionFormat(napi_env env, const napi_value arg, DescriptionFormat &format);
    static ClientStatus GetNetType(napi_env env, const napi_value arg, NetType &netType);
    static ClientStatus GetOrder(napi_env env, const napi_value arg, Order &order);
};
} // namespace OHOS::UpdateService
#endif // UPDATE_CLIENT_HELPER_H