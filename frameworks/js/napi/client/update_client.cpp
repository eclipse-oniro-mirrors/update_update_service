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

#include "update_client.h"

#include <thread>

#include "client_helper.h"
#include "iupdate_service.h"
#include "napi_util.h"
#include "update_helper.h"
#include "update_service_kits.h"
#include "update_session.h"
#include "updater_sa_ipc_interface_code.h"

using namespace std;

namespace OHOS {
namespace UpdateEngine {
napi_value UpdateClient::Napi::NapiOn(napi_env env, napi_callback_info info)
{
    CLIENT_LOGI("UpdateClient::Napi::NapiOn");
    UpdateClient *updater = UnwrapJsObject<UpdateClient>(env, info);
    PARAM_CHECK_NAPI_CALL(env, updater != nullptr, return nullptr, "Error get UpdateClient");
    return updater->On(env, info);
}

napi_value UpdateClient::Napi::NapiOff(napi_env env, napi_callback_info info)
{
    CLIENT_LOGI("UpdateClient::Napi::NapiOff");
    UpdateClient *updater = UnwrapJsObject<UpdateClient>(env, info);
    PARAM_CHECK_NAPI_CALL(env, updater != nullptr, return nullptr, "Error get UpdateClient");
    return updater->Off(env, info);
}

UpdateClient::UpdateClient(napi_env env, napi_value thisVar)
{
    CLIENT_LOGI("UpdateClient::UpdateClient");
    napi_ref thisReference = nullptr;
    napi_create_reference(env, thisVar, 1, &thisReference);
    sessionsMgr_ = std::make_shared<SessionManager>(env, thisReference);
}

UpdateClient::~UpdateClient()
{
    CLIENT_LOGI("UpdateClient::~UpdateClient");
    if (sessionsMgr_ != nullptr) {
        sessionsMgr_->Clear();
    }
}

napi_value UpdateClient::GetOnlineUpdater(napi_env env, napi_callback_info info)
{
    napi_value result;
    napi_create_int32(env, 0, &result);
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return nullptr, "Error get cb info");
    std::vector<std::pair<std::string, std::string>> paramInfos;
    paramInfos.push_back({"upgradeInfo", "UpgradeInfo"});
    PARAM_CHECK_NAPI_CALL(env, argc >= 1,
        ClientHelper::NapiThrowParamError(env, paramInfos);
        return nullptr, "Invalid param");
    PARAM_CHECK_NAPI_CALL(env, !isInit_, return result, "Has been init");

    ClientStatus ret = ClientHelper::GetUpgradeInfoFromArg(env, args[0], upgradeInfo_);
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS,
        ClientHelper::NapiThrowParamError(env, paramInfos);
        return nullptr, "Failed to get upgradeInfo param");

    std::thread initThread(&UpdateClient::InitCallback, this);
    initThread.detach();
    isInit_ = true;
    return result;
}

void UpdateClient::InitCallback()
{
    CLIENT_LOGI("InitCallback");
    UpdateCallbackInfo callback {
        [=](const BusinessError &businessError, const CheckResult &checkResult) {
            NotifyCheckVersionDone(businessError, checkResult);
        },
        [=](const EventInfo &eventInfo) {
            NotifyEventInfo(eventInfo);
        }
    };

    constexpr int32_t nextRetryDuration = 10;
    constexpr int32_t maxRetryTimes = 5;
    int32_t retryTimes = 0;
    do {
        int32_t ret = UpdateServiceKits::GetInstance().RegisterUpdateCallback(upgradeInfo_, callback);
        if (ret == INT_CALL_SUCCESS) {
            break;
        }

        if (retryTimes++ < maxRetryTimes) {
            CLIENT_LOGI("InitCallback fail, will retry after %{public}d milliseconds", nextRetryDuration);
            std::this_thread::sleep_for(std::chrono::milliseconds(nextRetryDuration));
        } else {
            CLIENT_LOGE("InitCallback fail after retry %{public}d times", retryTimes);
        }
    } while (retryTimes < maxRetryTimes);
}

napi_value UpdateClient::CheckNewVersion(napi_env env, napi_callback_info info)
{
    SessionParams sessionParams(SessionType::SESSION_CHECK_VERSION, CALLBACK_POSITION_ONE, false, true);
    napi_value ret = StartSession(env, info, sessionParams, [=](SessionType type, void *context) -> int {
        return UpdateServiceKits::GetInstance().CheckNewVersion(upgradeInfo_);
    });
    PARAM_CHECK(ret != nullptr, return nullptr, "Failed to start worker.");
    return ret;
}

napi_value UpdateClient::CancelUpgrade(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok && argc == 0, return nullptr, "Error get cb info");
    CLIENT_LOGI("CancelUpgrade");
    SessionParams sessionParams(SessionType::SESSION_CANCEL_UPGRADE, CALLBACK_POSITION_ONE, true);
    std::shared_ptr<UpdateSession> sess = nullptr;
    sess = std::make_shared<UpdateAsyncessionNoCallback>(this, sessionParams, argc);
    PARAM_CHECK_NAPI_CALL(env, sess != nullptr, return nullptr, "Failed to create update session");
    sessionsMgr_->AddSession(sess);
    napi_value retValue = sess->StartWork(
        env,
        args,
        [=](SessionType type, void *context) -> int {
            BusinessError *businessError = reinterpret_cast<BusinessError *>(context);
            return UpdateServiceKits::GetInstance().Cancel(upgradeInfo_,
                CAST_INT(UpdaterSaInterfaceCode::DOWNLOAD), *businessError);
        },
        nullptr);
    PARAM_CHECK(retValue != nullptr, sessionsMgr_->RemoveSession(sess->GetSessionId());
        return nullptr, "Failed to start worker.");
    return retValue;
}

template <typename T>
ClientStatus UpdateClient::ParseUpgOptions(napi_env env, napi_callback_info info, VersionDigestInfo &versionDigestInfo,
    T &options)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return ClientStatus::CLIENT_INVALID_PARAM, "Error get cb info");

    ClientStatus ret = ClientHelper::GetVersionDigestInfoFromArg(env, args[0], versionDigestInfo);
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS, return ClientStatus::CLIENT_INVALID_PARAM,
        "Failed to get versionDigestInfo param");

    ret = ClientHelper::GetOptionsFromArg(env, args[1], options);
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS, return ClientStatus::CLIENT_INVALID_PARAM,
        "Failed to get Options param");
    return ClientStatus::CLIENT_SUCCESS;
}

template <typename T>
ClientStatus UpdateClient::ParseUpgOptions(napi_env env, napi_callback_info info, T &options)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return ClientStatus::CLIENT_INVALID_PARAM, "Error get cb info");

    ClientStatus ret = ClientHelper::GetOptionsFromArg(env, args[0], options);
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS, return ClientStatus::CLIENT_INVALID_PARAM,
        "Failed to get Options param");
    return ClientStatus::CLIENT_SUCCESS;
}

napi_value UpdateClient::Download(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return nullptr, "Error get cb info");
    CLIENT_LOGI("Download");

    ClientStatus ret = ParseUpgOptions(env, info, versionDigestInfo_, downloadOptions_);
    std::vector<std::pair<std::string, std::string>> paramInfos;
    paramInfos.push_back({"versionDigestInfo", "VersionDigestInfo"});
    paramInfos.push_back({"downloadOptions", "DownloadOptions"});
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS,
        ClientHelper::NapiThrowParamError(env, paramInfos);
        return nullptr, "Failed to get Download param");

    SessionParams sessionParams(SessionType::SESSION_DOWNLOAD, CALLBACK_POSITION_THREE, true);
    napi_value retValue = StartSession(env, info, sessionParams, [=](SessionType type, void *context) -> int {
        BusinessError *businessError = reinterpret_cast<BusinessError *>(context);
        return UpdateServiceKits::GetInstance().Download(
            upgradeInfo_, versionDigestInfo_, downloadOptions_, *businessError);
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to start worker.");
    return retValue;
}

napi_value UpdateClient::PauseDownload(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return nullptr, "Error get cb info");

    ClientStatus ret = ParseUpgOptions(env, info, versionDigestInfo_, pauseDownloadOptions_);
    std::vector<std::pair<std::string, std::string>> paramInfos;
    paramInfos.push_back({ "versionDigestInfo", "VersionDigestInfo" });
    paramInfos.push_back({ "pauseDownloadOptions", "PauseDownloadOptions" });
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS,
        ClientHelper::NapiThrowParamError(env, paramInfos);
        return nullptr, "Failed to get pauseDownloadOptions param");

    SessionParams sessionParams(SessionType::SESSION_PAUSE_DOWNLOAD, CALLBACK_POSITION_THREE, true);
    napi_value retValue = StartSession(env, info, sessionParams, [=](SessionType type, void *context) -> int {
        BusinessError *businessError = reinterpret_cast<BusinessError *>(context);
        return UpdateServiceKits::GetInstance().PauseDownload(
            upgradeInfo_, versionDigestInfo_, pauseDownloadOptions_, *businessError);
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to start worker.");
    return retValue;
}

napi_value UpdateClient::ResumeDownload(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return nullptr, "Error get cb info");

    ClientStatus ret = ParseUpgOptions(env, info, versionDigestInfo_, resumeDownloadOptions_);
    std::vector<std::pair<std::string, std::string>> paramInfos;
    paramInfos.push_back({"versionDigestInfo", "VersionDigestInfo"});
    paramInfos.push_back({"resumeDownloadOptions", "ResumeDownloadOptions"});
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS,
        ClientHelper::NapiThrowParamError(env, paramInfos);
        return nullptr, "Failed to get resumeDownloadOptions param");

    SessionParams sessionParams(SessionType::SESSION_RESUME_DOWNLOAD, CALLBACK_POSITION_THREE, true);
    napi_value retValue = StartSession(env, info, sessionParams, [=](SessionType type, void *context) -> int {
        BusinessError *businessError = reinterpret_cast<BusinessError *>(context);
        return UpdateServiceKits::GetInstance().ResumeDownload(
            upgradeInfo_, versionDigestInfo_, resumeDownloadOptions_, *businessError);
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to start worker.");
    return retValue;
}

napi_value UpdateClient::Upgrade(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return nullptr, "Error get cb info");

    ClientStatus ret = ParseUpgOptions(env, info, versionDigestInfo_, upgradeOptions_);
    std::vector<std::pair<std::string, std::string>> paramInfos;
    paramInfos.push_back({"versionDigestInfo", "VersionDigestInfo"});
    paramInfos.push_back({"upgradeOptions", "UpgradeOptions"});
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS,
        ClientHelper::NapiThrowParamError(env, paramInfos);
        return nullptr, "Failed to get upgradeOptions param");

    SessionParams sessionParams(SessionType::SESSION_UPGRADE, CALLBACK_POSITION_THREE, true);
    napi_value retValue = StartSession(env, info, sessionParams, [=](SessionType type, void *context) -> int {
#ifndef UPDATER_API_TEST
        BusinessError *businessError = reinterpret_cast<BusinessError *>(context);
        return UpdateServiceKits::GetInstance().Upgrade(
            upgradeInfo_, versionDigestInfo_, upgradeOptions_, *businessError);
#else
        return INT_CALL_SUCCESS;
#endif
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to start worker.");
    return retValue;
}

napi_value UpdateClient::ClearError(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return nullptr, "Error get cb info");

    ClientStatus ret = ParseUpgOptions(env, info, versionDigestInfo_, clearOptions_);
    std::vector<std::pair<std::string, std::string>> paramInfos;
    paramInfos.push_back({ "versionDigestInfo", "VersionDigestInfo" });
    paramInfos.push_back({ "clearOptions", "ClearOptions" });
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS,
        ClientHelper::NapiThrowParamError(env, paramInfos);
        return nullptr, "Failed to get clearOptions param");

    SessionParams sessionParams(SessionType::SESSION_CLEAR_ERROR, CALLBACK_POSITION_THREE, true);
    napi_value retValue = StartSession(env, info, sessionParams,
        [=](SessionType type, void *context) -> int {
            BusinessError *businessError = reinterpret_cast<BusinessError*>(context);
            return UpdateServiceKits::GetInstance().ClearError(
                upgradeInfo_, versionDigestInfo_, clearOptions_, *businessError);
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to start worker.");
    return retValue;
}

napi_value UpdateClient::TerminateUpgrade(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return nullptr, "Error get cb info");

    SessionParams sessionParams(SessionType::SESSION_TERMINATE_UPGRADE, CALLBACK_POSITION_ONE, true);
    napi_value retValue = StartSession(env, info, sessionParams, [=](SessionType type, void *context) -> int {
        BusinessError *businessError = reinterpret_cast<BusinessError *>(context);
        return UpdateServiceKits::GetInstance().TerminateUpgrade(upgradeInfo_, *businessError);
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to start worker.");
    return retValue;
}

napi_value UpdateClient::SetUpgradePolicy(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return nullptr, "Error get cb info");

    ClientStatus ret = ClientHelper::GetUpgradePolicyFromArg(env, args[0], upgradePolicy_);
    std::vector<std::pair<std::string, std::string>> paramInfos;
    paramInfos.push_back({"upgradePolicy", "UpgradePolicy"});
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS,
        ClientHelper::NapiThrowParamError(env, paramInfos);
        return nullptr, "Failed to get upgradePolicy param");

    SessionParams sessionParams(SessionType::SESSION_SET_POLICY, CALLBACK_POSITION_TWO, true);
    napi_value retValue = StartSession(env, info, sessionParams,
        [&](SessionType type, void *context) -> int {
            BusinessError *businessError = reinterpret_cast<BusinessError*>(context);
            return UpdateServiceKits::GetInstance().SetUpgradePolicy(upgradeInfo_, upgradePolicy_, *businessError);
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to SetUpgradePolicy.");
    return retValue;
}

napi_value UpdateClient::GetUpgradePolicy(napi_env env, napi_callback_info info)
{
    SessionParams sessionParams(SessionType::SESSION_GET_POLICY, CALLBACK_POSITION_ONE, true);
    napi_value retValue = StartSession(env, info, sessionParams,
        [=](SessionType type, void *context) -> int {
            BusinessError *businessError = reinterpret_cast<BusinessError*>(context);
            return UpdateServiceKits::GetInstance().GetUpgradePolicy(upgradeInfo_, upgradePolicy_, *businessError);
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to UpgradeVersion.");
    return retValue;
}

napi_value UpdateClient::GetNewVersionInfo(napi_env env, napi_callback_info info)
{
    SessionParams sessionParams(SessionType::SESSION_GET_NEW_VERSION, CALLBACK_POSITION_ONE, true);
    napi_value retValue = StartSession(env, info, sessionParams,
        [=](SessionType type, void *context) -> int {
            BusinessError *businessError = reinterpret_cast<BusinessError*>(context);
            return UpdateServiceKits::GetInstance().GetNewVersionInfo(upgradeInfo_, newVersionInfo_, *businessError);
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to GetNewVersionInfo.");
    return retValue;
}

napi_value UpdateClient::GetNewVersionDescription(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return nullptr, "Error get cb info");
    CLIENT_LOGI("GetNewVersionDescription");

    ClientStatus ret = ParseUpgOptions(env, info, versionDigestInfo_, descriptionOptions_);
    std::vector<std::pair<std::string, std::string>> paramInfos;
    paramInfos.push_back({"versionDigestInfo", "VersionDigestInfo"});
    paramInfos.push_back({"descriptionOptions", "DescriptionOptions"});
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS,
        ClientHelper::NapiThrowParamError(env, paramInfos);
        return nullptr, "Failed to get descriptionOptions param");

    SessionParams sessionParams(SessionType::SESSION_GET_NEW_VERSION_DESCRIPTION, CALLBACK_POSITION_THREE, true);
    napi_value retValue = StartSession(env, info, sessionParams, [=](SessionType type, void *context) -> int {
        BusinessError *businessError = reinterpret_cast<BusinessError *>(context);
        return UpdateServiceKits::GetInstance().GetNewVersionDescription(
            upgradeInfo_, versionDigestInfo_, descriptionOptions_, newVersionDescriptionInfo_, *businessError);
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to start worker.");
    return retValue;
}

napi_value UpdateClient::GetCurrentVersionInfo(napi_env env, napi_callback_info info)
{
    SessionParams sessionParams(SessionType::SESSION_GET_CUR_VERSION, CALLBACK_POSITION_ONE, true);
    napi_value retValue = StartSession(env, info, sessionParams,
        [=](SessionType type, void *context) -> int {
            BusinessError *businessError = reinterpret_cast<BusinessError*>(context);
            return UpdateServiceKits::GetInstance().GetCurrentVersionInfo(upgradeInfo_, currentVersionInfo_,
                *businessError);
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to GetCurrentVersionInfo.");
    return retValue;
}

napi_value UpdateClient::GetCurrentVersionDescription(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGC;
    napi_value args[MAX_ARGC] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    PARAM_CHECK_NAPI_CALL(env, status == napi_ok, return nullptr, "Error get cb info");
    CLIENT_LOGI("GetCurrentVersionDescription");

    ClientStatus ret = ParseUpgOptions(env, info, descriptionOptions_);
    std::vector<std::pair<std::string, std::string>> paramInfos;
    paramInfos.push_back({"descriptionOptions", "DescriptionOptions"});
    PARAM_CHECK(ret == ClientStatus::CLIENT_SUCCESS,
        ClientHelper::NapiThrowParamError(env, paramInfos);
        return nullptr, "Failed to get descriptionOptions param");

    SessionParams sessionParams(SessionType::SESSION_GET_CUR_VERSION_DESCRIPTION, CALLBACK_POSITION_TWO, true);
    napi_value retValue = StartSession(env, info, sessionParams, [=](SessionType type, void *context) -> int {
        BusinessError *businessError = reinterpret_cast<BusinessError *>(context);
        return UpdateServiceKits::GetInstance().GetCurrentVersionDescription(
            upgradeInfo_, descriptionOptions_, currentVersionDescriptionInfo_, *businessError);
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to start worker.");
    return retValue;
}

napi_value UpdateClient::GetTaskInfo(napi_env env, napi_callback_info info)
{
    SessionParams sessionParams(SessionType::SESSION_GET_TASK_INFO, CALLBACK_POSITION_ONE, true);
    napi_value retValue = StartSession(env, info, sessionParams,
        [=](SessionType type, void *context) -> int {
            BusinessError *businessError = reinterpret_cast<BusinessError*>(context);
            return UpdateServiceKits::GetInstance().GetTaskInfo(upgradeInfo_, taskInfo_, *businessError);
    });
    PARAM_CHECK(retValue != nullptr, return nullptr, "Failed to GetTaskInfo.");
    return retValue;
}

void UpdateClient::NotifyCheckVersionDone(const BusinessError &businessError, const CheckResult &checkResult)
{
    CLIENT_LOGI("NotifyCheckVersionDone businessError %{public}d isExistNewVersion %{public}d",
                static_cast<int32_t>(businessError.errorNum), checkResult.isExistNewVersion);
    checkResult_ = checkResult;
    sessionsMgr_->Emit("checkNewVersion", businessError);
}

void UpdateClient::GetUpdateResult(SessionType type, UpdateResult &result)
{
    CLIENT_LOGI("GetUpdateResult type %{public}d", type);
    result.type = type;
    switch (type) {
        case SessionType::SESSION_CHECK_VERSION:
            result.result.checkResult = &checkResult_;
            result.buildJSObject = ClientHelper::BuildCheckResult;
            break;
        case SessionType::SESSION_GET_NEW_VERSION:
            result.result.newVersionInfo = &newVersionInfo_;
            result.buildJSObject = ClientHelper::BuildNewVersionInfo;
            break;
        case SessionType::SESSION_GET_NEW_VERSION_DESCRIPTION:
            result.result.versionDescriptionInfo = &newVersionDescriptionInfo_;
            result.buildJSObject = ClientHelper::BuildVersionDescriptionInfo;
            break;
        case SessionType::SESSION_GET_TASK_INFO:
            result.result.taskInfo = &taskInfo_;
            result.buildJSObject = ClientHelper::BuildTaskInfo;
            break;
        case SessionType::SESSION_GET_CUR_VERSION:
            result.result.currentVersionInfo = &currentVersionInfo_;
            result.buildJSObject = ClientHelper::BuildCurrentVersionInfo;
            break;
        case SessionType::SESSION_GET_CUR_VERSION_DESCRIPTION:
            result.result.versionDescriptionInfo = &currentVersionDescriptionInfo_;
            result.buildJSObject = ClientHelper::BuildVersionDescriptionInfo;
            break;
        case SessionType::SESSION_GET_POLICY:
            result.result.upgradePolicy = &upgradePolicy_;
            result.buildJSObject = ClientHelper::BuildUpgradePolicy;
            break;
        default:
            result.buildJSObject = ClientHelper::BuildUndefinedStatus;
            break;
    }
}
} // namespace UpdateEngine
} // namespace OHOS
