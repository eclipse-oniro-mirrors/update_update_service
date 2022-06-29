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

#ifndef UPDATE_CLIENT_H
#define UPDATE_CLIENT_H

#include "client_helper.h"
#include "iupdater.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_util.h"
#include "node_api.h"
#include "node_api_types.h"
#include "session_manager.h"
#include "update_helper.h"
#include "update_service_kits.h"
#include "update_session.h"

namespace OHOS {
namespace UpdateEngine {
class UpdateClient : public IUpdater {
public:
    class Napi {
    public:
        static constexpr const char *FUNCTION_ON = "on";
        static constexpr const char *FUNCTION_OFF = "off";

        static napi_value NapiOn(napi_env env, napi_callback_info info);
        static napi_value NapiOff(napi_env env, napi_callback_info info);
    };

    UpdateClient(napi_env env, napi_value thisVar);
    ~UpdateClient() override;

    // Obtain the online updater engine and return it through the sync API.
    napi_value GetOnlineUpdater(napi_env env, napi_callback_info info);

    // Asynchronous API
    napi_value CheckNewVersion(napi_env env, napi_callback_info info);
    napi_value SetUpdatePolicy(napi_env env, napi_callback_info info);
    napi_value GetUpdatePolicy(napi_env env, napi_callback_info info);
    napi_value GetNewVersionInfo(napi_env env, napi_callback_info info);
    napi_value GetCurrentVersionInfo(napi_env env, napi_callback_info info);
    napi_value GetTaskInfo(napi_env env, napi_callback_info info);

    napi_value GetOtaStatus(napi_env env, napi_callback_info info);
    napi_value ApplyNewVersion(napi_env env, napi_callback_info info);
    napi_value RebootAndClean(napi_env env, napi_callback_info info);

    // Event mode, which is used to send the result to the JS.
    napi_value CancelUpgrade(napi_env env, napi_callback_info info);
    napi_value DownloadVersion(napi_env env, napi_callback_info info);
    napi_value ResumeDownload(napi_env env, napi_callback_info info);
    napi_value PauseDownload(napi_env env, napi_callback_info info);
    napi_value Upgrade(napi_env env, napi_callback_info info);
    napi_value ClearError(napi_env env, napi_callback_info info);
    napi_value TerminateUpgrade(napi_env env, napi_callback_info info);

    napi_value VerifyUpdatePackage(napi_env env, napi_callback_info info);

    int32_t GetUpdateResult(SessionType type, UpdateResult &result) override;

    // Notify the session.
    void NotifyCheckVersionDone(const BusinessError &businessError, const CheckResultEx &checkResultEx);
    void NotifyDownloadProgress(const BusinessError &businessError, const Progress &progress);
    void NotifyUpgradeProgresss(const BusinessError &businessError, const Progress &progress);
    void NotifyVerifyProgresss(int32_t retCode, uint32_t percent);

    #ifdef UPDATER_UT
    UpdateSession *GetUpdateSession(uint32_t sessId)
    {
    #ifndef UPDATER_API_TEST
        std::lock_guard<std::mutex> guard(sessionMutex_);
    #endif
        auto iter = sessions_.find(sessId);
        if (iter != sessions_.end()) {
            return iter->second.get();
        }
        return nullptr;
    }
    #endif
private:
    template <typename T>
    ClientStatus ParseUpgOptions(napi_env env, napi_callback_info info, VersionDigestInfo &versionDigestInfo,
        T &options);
#ifndef UPDATER_API_TEST
    std::mutex sessionMutex_;
#endif
    bool isInit_ = false;
    int32_t result_ = 0;
    std::string upgradeFile_;
    std::string certsFile_;
    UpgradeInfo upgradeInfo_ {};
    UpdatePolicy updatePolicy_ {};
    Progress progress_ {};
    Progress verifyProgress_ {};
    VersionInfo versionInfo_ {};
    CheckResultEx checkResultEx_ {};
    NewVersionInfo newVersionInfo_ {};
    CurrentVersionInfo currentVersionInfo_ {};
    TaskInfo taskInfo_ {};
    OtaStatus otaStatus_ {};
    VersionDigestInfo versionDigestInfo_ {};
    UpgradeOptions upgradeOptions_;
    ClearOptions clearOptions_ {};
    DownloadOptions downloadOptions_ {};
    ResumeDownloadOptions resumeDownloadOptions_ {};
    PauseDownloadOptions pauseDownloadOptions_ {};
};

#ifdef UPDATER_UT
napi_value UpdateClientInit(napi_env env, napi_value exports);
#endif
} // namespace UpdateEngine
} // namespace OHOS
#endif // UPDATE_CLIENT_H