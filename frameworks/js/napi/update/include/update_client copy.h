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

#ifndef UPDATE_CLIENT_H
#define UPDATE_CLIENT_H

#include "check_result.h"
#include "current_version_info.h"
#include "iupdater.h"
#include "new_version_info.h"
#include "progress.h"
#include "task_info.h"
#include "version_description_info.h"

namespace OHOS::UpdateService {
class UpdateClient : public IUpdater {
public:
    DECLARE_CLASS_NAME(UpdateClient);
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
    napi_value SetUpgradePolicy(napi_env env, napi_callback_info info);
    napi_value GetUpgradePolicy(napi_env env, napi_callback_info info);
    napi_value GetNewVersionInfo(napi_env env, napi_callback_info info);
    napi_value GetNewVersionDescription(napi_env env, napi_callback_info info);
    napi_value GetCurrentVersionInfo(napi_env env, napi_callback_info info);
    napi_value GetCurrentVersionDescription(napi_env env, napi_callback_info info);
    napi_value GetTaskInfo(napi_env env, napi_callback_info info);

    // Event mode, which is used to send the result to the JS.
    napi_value CancelUpgrade(napi_env env, napi_callback_info info);
    napi_value Download(napi_env env, napi_callback_info info);
    napi_value ResumeDownload(napi_env env, napi_callback_info info);
    napi_value PauseDownload(napi_env env, napi_callback_info info);
    napi_value Upgrade(napi_env env, napi_callback_info info);
    napi_value ClearError(napi_env env, napi_callback_info info);
    napi_value TerminateUpgrade(napi_env env, napi_callback_info info);

    void GetUpdateResult(uint32_t type, UpdateResult &result) override;

    // Notify the session.
    void NotifyDownloadProgress(const BusinessError &businessError, const Progress &progress);
    void NotifyUpgradeProgresss(const BusinessError &businessError, const Progress &progress);

    #ifdef UPDATER_UT
    UpdateSession *GetUpdateSession(uint32_t sessId)
    {
        std::lock_guard<std::mutex> guard(sessionMutex_);
        auto iter = sessions_.find(sessId);
        if (iter != sessions_.end()) {
            return iter->second.get();
        }
        return nullptr;
    }
    #endif

protected:
    void RegisterCallback() override;
    void UnRegisterCallback() override;

private:
    template <typename T>
    ClientStatus ParseUpgOptions(napi_env env, napi_callback_info info, VersionDigestInfo &versionDigestInfo,
        T &options);
    template <typename T>
    ClientStatus ParseUpgOptions(napi_env env, napi_callback_info info, T &options);

private:
    std::mutex sessionMutex_;
    bool isInit_ = false;
    std::string upgradeFile_;
    std::string certsFile_;
    UpgradeInfo upgradeInfo_ {};
    UpgradePolicy upgradePolicy_ {};
    Progress progress_ {};
    Progress verifyProgress_ {};
    CheckResult checkResult_ {};
    NewVersionInfo newVersionInfo_ {};
    VersionDescriptionInfo newVersionDescriptionInfo_ {};
    CurrentVersionInfo currentVersionInfo_ {};
    VersionDescriptionInfo currentVersionDescriptionInfo_ {};
    TaskInfo taskInfo_ {};
    VersionDigestInfo versionDigestInfo_ {};
    DescriptionOptions descriptionOptions_ {};
    UpgradeOptions upgradeOptions_;
    ClearOptions clearOptions_ {};
    DownloadOptions downloadOptions_ {};
    ResumeDownloadOptions resumeDownloadOptions_ {};
    PauseDownloadOptions pauseDownloadOptions_ {};
};

#ifdef UPDATER_UT
napi_value UpdateClientInit(napi_env env, napi_value exports);
#endif
} // namespace OHOS::UpdateService
#endif // UPDATE_CLIENT_H