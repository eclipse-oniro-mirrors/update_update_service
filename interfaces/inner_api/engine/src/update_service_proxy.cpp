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

#include <string_ex.h>
#include "update_service_proxy.h"

#include "securec.h"

#include "message_parcel_helper.h"
#include "update_define.h"
#include "update_define.h"
#include "update_log.h"
#include "updater_sa_ipc_interface_code.h"

namespace OHOS::UpdateEngine {
#define RETURN_WHEN_REMOTE_NULL(remote) \
    ENGINE_CHECK((remote) != nullptr, return INT_CALL_IPC_ERR, "Can not get remote")

#define IPC_RESULT_TO_CALL_RESULT(result)           \
    if ((result) == ERR_NONE) {                     \
        result = INT_CALL_SUCCESS;                  \
    } else if ((result) >= CALL_RESULT_OFFSET) {    \
        result = (result) - CALL_RESULT_OFFSET;     \
    } else {                                        \
        result = INT_CALL_IPC_ERR;                  \
    }

#define RETURN_WHEN_TOKEN_WRITE_FAIL(data)                             \
    if (!(data).WriteInterfaceToken(GetDescriptor())) {                \
        ENGINE_LOGE("UpdateServiceProxy WriteInterfaceToken fail");    \
        return INT_CALL_IPC_ERR;                                       \
    }

#define RETURN_FAIL_WHEN_REMOTE_ERR(methodName, res)                             \
    do {                                                                         \
        ENGINE_LOGI("%{public}s is %{public}d", methodName, res);                \
        IPC_RESULT_TO_CALL_RESULT(res);                                          \
        ENGINE_CHECK((res) == INT_CALL_SUCCESS, return (res), "Transact error"); \
    } while (0)

int32_t UpdateServiceProxy::RegisterUpdateCallback(const UpgradeInfo &info, const sptr<IUpdateCallback> &updateCallback)
{
    ENGINE_CHECK(updateCallback != nullptr, return INT_PARAM_ERR, "Invalid param");
    ENGINE_LOGI("UpdateServiceProxy::RegisterUpdateCallback");

    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);
    bool res = data.WriteRemoteObject(updateCallback->AsObject());
    ENGINE_CHECK(res, return INT_CALL_IPC_ERR, "RegisterUpdateCallback error, WriteRemoteObject fail");

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::REGISTER_CALLBACK), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::RegisterUpdateCallback", ret);
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::UnregisterUpdateCallback(const UpgradeInfo &info)
{
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::UNREGISTER_CALLBACK), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::UnregisterUpdateCallback", ret);
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::CheckNewVersion(const UpgradeInfo &info, BusinessError &businessError,
    CheckResult &checkResult)
{
    ENGINE_LOGI("UpdateServiceProxy::CheckNewVersion");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::CHECK_VERSION), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::CheckNewVersion", ret);
    CheckResult result;
    BusinessError error;
    MessageParcelHelper::ReadBusinessError(reply, error);
    MessageParcelHelper::ReadCheckResult(reply, result);
    checkResult = result;
    businessError = error;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::Download(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const DownloadOptions &downloadOptions, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::Download");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);
    MessageParcelHelper::WriteVersionDigestInfo(data, versionDigestInfo);
    MessageParcelHelper::WriteDownloadOptions(data, downloadOptions);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::DOWNLOAD), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::Download", ret);
    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::PauseDownload(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const PauseDownloadOptions &pauseDownloadOptions, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::PauseDownload");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);
    MessageParcelHelper::WriteVersionDigestInfo(data, versionDigestInfo);
    MessageParcelHelper::WritePauseDownloadOptions(data, pauseDownloadOptions);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::PAUSE_DOWNLOAD), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::ResumeDownload", ret);
    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::ResumeDownload(const UpgradeInfo &info, const VersionDigestInfo &versionDigestInfo,
    const ResumeDownloadOptions &resumeDownloadOptions, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::ResumeDownload");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);
    MessageParcelHelper::WriteVersionDigestInfo(data, versionDigestInfo);
    MessageParcelHelper::WriteResumeDownloadOptions(data, resumeDownloadOptions);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::RESUME_DOWNLOAD), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::ResumeDownload", ret);
    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::Upgrade(const UpgradeInfo &info, const VersionDigestInfo &versionDigest,
    const UpgradeOptions &upgradeOptions, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::Upgrade, versionDigest %{public}s upgradeOptions %{public}d",
        versionDigest.versionDigest.c_str(),
        upgradeOptions.order);
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);
    MessageParcelHelper::WriteVersionDigestInfo(data, versionDigest);
    MessageParcelHelper::WriteUpgradeOptions(data, upgradeOptions);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::UPGRADE), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::Upgrade", ret);
    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::ClearError(const UpgradeInfo &info, const VersionDigestInfo &versionDigest,
    const ClearOptions &clearOptions, BusinessError &businessError)
{
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);
    MessageParcelHelper::WriteVersionDigestInfo(data, versionDigest);
    MessageParcelHelper::WriteClearOptions(data, clearOptions);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::CLEAR_ERROR), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::ClearError", ret);
    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::TerminateUpgrade(const UpgradeInfo &info, BusinessError &businessError)
{
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::TERMINATE_UPGRADE), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::TerminateUpgrade", ret);
    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::GetNewVersionInfo(const UpgradeInfo &info, NewVersionInfo &newVersionInfo,
    BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::GetNewVersionInfo");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::GET_NEW_VERSION), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::GetNewVersionInfo", ret);

    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    NewVersionInfo remoteNewVersionInfo;
    MessageParcelHelper::ReadNewVersionInfo(reply, remoteNewVersionInfo);
    newVersionInfo = remoteNewVersionInfo;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::GetNewVersionDescription(const UpgradeInfo &info,
    const VersionDigestInfo &versionDigestInfo, const DescriptionOptions &descriptionOptions,
    VersionDescriptionInfo &newVersionDescriptionInfo, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::GetNewVersionDescription");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);
    MessageParcelHelper::WriteVersionDigestInfo(data, versionDigestInfo);
    MessageParcelHelper::WriteDescriptionOptions(data, descriptionOptions);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::GET_NEW_VERSION_DESCRIPTION),
        data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::GetNewVersionDescription", ret);

    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    VersionDescriptionInfo remoteVersionDescriptionInfo;
    MessageParcelHelper::ReadVersionDescriptionInfo(reply, remoteVersionDescriptionInfo);
    newVersionDescriptionInfo = remoteVersionDescriptionInfo;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::GetCurrentVersionInfo(const UpgradeInfo &info, CurrentVersionInfo &currentVersionInfo,
    BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::GetCurrentVersionInfo");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::GET_CURRENT_VERSION), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::GetCurrentVersionInfo", ret);

    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    CurrentVersionInfo remoteCurrentVersionInfo;
    MessageParcelHelper::ReadCurrentVersionInfo(reply, remoteCurrentVersionInfo);
    currentVersionInfo = remoteCurrentVersionInfo;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::GetCurrentVersionDescription(const UpgradeInfo &info,
    const DescriptionOptions &descriptionOptions, VersionDescriptionInfo &currentVersionDescriptionInfo,
    BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::GetCurrentVersionDescription");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);
    MessageParcelHelper::WriteDescriptionOptions(data, descriptionOptions);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::GET_CURRENT_VERSION_DESCRIPTION),
        data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::GetCurrentVersionDescription", ret);

    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    VersionDescriptionInfo remoteVersionDescriptionInfo;
    MessageParcelHelper::ReadVersionDescriptionInfo(reply, remoteVersionDescriptionInfo);
    currentVersionDescriptionInfo = remoteVersionDescriptionInfo;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::GetTaskInfo(const UpgradeInfo &info, TaskInfo &taskInfo, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::GetTaskInfo");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::GET_TASK_INFO), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::GetTaskInfo", ret);

    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    TaskInfo remoteTaskInfo;
    MessageParcelHelper::ReadTaskInfo(reply, remoteTaskInfo);
    taskInfo = remoteTaskInfo;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::SetUpgradePolicy(const UpgradeInfo &info, const UpgradePolicy &policy,
    BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::SetUpgradePolicy");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);
    MessageParcelHelper::WriteUpgradePolicy(data, policy);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::SET_POLICY), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::SetUpgradePolicy", ret);

    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::GetUpgradePolicy(const UpgradeInfo &info, UpgradePolicy &policy,
    BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::GetUpgradePolicy");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::GET_POLICY), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::GetUpgradePolicy", ret);

    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    UpgradePolicy remoteUpgradePolicy;
    MessageParcelHelper::ReadUpgradePolicy(reply, remoteUpgradePolicy);
    policy = remoteUpgradePolicy;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::Cancel(const UpgradeInfo &info, int32_t service, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::Cancel");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);
    data.WriteInt32(static_cast<int32_t>(service));

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::CANCEL), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::Cancel", ret);

    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::FactoryReset(BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::FactoryReset");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);

    MessageParcel reply;
    MessageOption option;
    int32_t ret = ERR_NONE; // IPC errCode: defined in ipc_types.h
#ifndef UPDATER_UT
    ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::FACTORY_RESET), data, reply, option);
#endif
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::FactoryReset", ret);

    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::ApplyNewVersion(const UpgradeInfo &info, const std::string &miscFile,
    const std::string &packageName, BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::ApplyNewVersion");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    MessageParcelHelper::WriteUpgradeInfo(data, info);
    data.WriteString16(Str8ToStr16(miscFile));
    data.WriteString16(Str8ToStr16(packageName));

    MessageParcel reply;
    MessageOption option;
    int32_t ret = ERR_NONE;
#ifndef UPDATER_UT
    ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::APPLY_NEW_VERSION), data, reply, option);
#endif
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::ApplyNewVersion", ret);

    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    return INT_CALL_SUCCESS;
}

int32_t UpdateServiceProxy::VerifyUpgradePackage(const std::string &packagePath, const std::string &keyPath,
    BusinessError &businessError)
{
    ENGINE_LOGI("UpdateServiceProxy::VerifyUpgradePackage");
    auto remote = Remote();
    RETURN_WHEN_REMOTE_NULL(remote);

    MessageParcel data;
    RETURN_WHEN_TOKEN_WRITE_FAIL(data);
    data.WriteString16(Str8ToStr16(packagePath));
    data.WriteString16(Str8ToStr16(keyPath));

    MessageParcel reply;
    MessageOption option;
    int32_t ret = remote->SendRequest(CAST_UINT(UpdaterSaInterfaceCode::VERIFY_UPGRADE_PACKAGE), data, reply, option);
    RETURN_FAIL_WHEN_REMOTE_ERR("UpdateServiceProxy::VerifyUpgradePackage", ret);

    BusinessError remoteBusinessError;
    MessageParcelHelper::ReadBusinessError(reply, remoteBusinessError);
    businessError = remoteBusinessError;
    return INT_CALL_SUCCESS;
}
} // namespace OHOS::UpdateEngine
