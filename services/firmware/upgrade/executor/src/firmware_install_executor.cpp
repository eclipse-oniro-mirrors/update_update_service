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

#include "firmware_install_executor.h"

#include <thread>

#include "ffrt.h"

#include "event_id.h"
#include "firmware_callback_utils.h"
#include "firmware_component_operator.h"
#include "firmware_constant.h"
#include "firmware_install_factory.h"
#include "firmware_log.h"
#include "firmware_task_operator.h"
#include "firmware_update_helper.h"

namespace OHOS {
namespace UpdateService {
void FirmwareInstallExecutor::Execute()
{
    FIRMWARE_LOGI("FirmwareInstallExecutor::Execute");
    ffrt::submit([executor = shared_from_this()] { executor->DoInstall(); });
}

void FirmwareInstallExecutor::DoInstall()
{
    FirmwareComponentOperator().QueryAll(components_);
    FIRMWARE_LOGI("FirmwareInstallExecutor DoInstall installType: %{public}d, component num: %{public}d",
        CAST_INT(installType_), CAST_INT(components_.size()));
    if (components_.size() == 0) {
        Progress progress;
        if (installType_ == InstallType::SYS_INSTALLER || installType_ == InstallType::STREAM_INSTALLLER) {
            progress.status = UpgradeStatus::INSTALL_FAIL;
        } else {
            progress.status = UpgradeStatus::UPDATE_FAIL;
        }
        
        progress.endReason = "no task";
        installCallbackInfo_.progress = progress;
        if (installCallback_.installCallback == nullptr) {
            FIRMWARE_LOGE("FirmwareInstallExecutor DoInstall installCallback is null");
            return;
        }
        installCallback_.installCallback(installCallbackInfo_);
        return;
    }

    GetTask();
    Progress progress;
    if (installType_ == InstallType::SYS_INSTALLER || installType_ == InstallType::STREAM_INSTALLLER) {
        progress.status = UpgradeStatus::INSTALLING;
    } else if (installType_ == InstallType::UPDATER) {
        progress.status = UpgradeStatus::UPDATING;
    } else {
        FIRMWARE_LOGI("installType:%{public}d is illegal", CAST_INT(installType_));
    }

    FirmwareTaskOperator().UpdateProgressByTaskId(tasks_.taskId, progress.status, progress.percent);
    for (FirmwareComponent &component : components_) {
        FirmwareComponentOperator().UpdateProgressByUrl(component.url, progress.status, progress.percent);
    }

    StartInstall();
}

void FirmwareInstallExecutor::StartInstall()
{
    FirmwareInstallCallback cb{
        [=](const FirmwareComponent &component) {
            HandleInstallProgress(component);
        },
        [=](bool result, const ErrorMessage &errMsg, UpgradeStatus status) {
            HandleInstallResult(result, errMsg, status);
        },
        [=](UpgradeStatus status) {
            FIRMWARE_LOGI("update start status :%{public}d", CAST_INT(status));
            DelayedSingleton<FirmwareCallbackUtils>::GetInstance()->NotifyEvent(tasks_.taskId,
                EventId::EVENT_UPGRADE_START, status);
        }
    };

    std::shared_ptr<FirmwareInstall> executor = InstallFactory::GetInstance(installType_);
    if (executor == nullptr) {
        FIRMWARE_LOGE("get install pointer fail");
        return;
    }
    executor->StartInstall(components_, cb);
}

void FirmwareInstallExecutor::GetTask()
{
    if (!tasks_.isExistTask) {
        FirmwareTaskOperator().QueryTask(tasks_);
    }
}

void FirmwareInstallExecutor::HandleInstallProgress(const FirmwareComponent &component)
{
    FIRMWARE_LOGI("FirmwareInstallExecutor::HandleInstallProgress status:%{public}d,progress:%{public}d, "
        "recordPoint:%{public}" PRId64, component.status, component.progress, component.recordPoint);
    FirmwareComponentOperator().UpdateProgressByUrl(component.url, component.status, component.progress);
    // 避免安装失败重复提交事件, 进度回调状态置为安装中
    taskProgress_.status = UpgradeStatus::INSTALLING;
    taskProgress_.percent = component.progress;
    installCallbackInfo_.progress = taskProgress_;
    if (installCallback_.installCallback == nullptr) {
        FIRMWARE_LOGE("FirmwareInstallExecutor HandleInstallProgress installCallback is null");
        return;
    }
    //流式升级只更新recordPoint还原点不实时更新task的status，避免异步安装过程覆盖结果
    if (installType_ == InstallType::STREAM_INSTALLLER) {
        FirmwareComponentOperator().UpdateRecordPointByUrl(component.url, component.recordPoint);
    } else {
        FirmwareTaskOperator().UpdateProgressByTaskId(tasks_.taskId, taskProgress_.status, taskProgress_.percent);
    }

    installCallback_.installCallback(installCallbackInfo_);
}

void FirmwareInstallExecutor::HandleInstallResult(const bool result, const ErrorMessage &errMsg,
    const UpgradeStatus &status)
{
    FIRMWARE_LOGI("FirmwareInstallExecutor::HandleInstallResult, result =%{public}d", result);
    if (result) {
        taskProgress_.status = UpgradeStatus::INSTALL_SUCCESS;
        taskProgress_.percent = Firmware::ONE_HUNDRED;
    } else {
        if (installType_ == InstallType::SYS_INSTALLER) {
            taskProgress_.status = UpgradeStatus::INSTALL_FAIL;
        } else if (installType_ == InstallType::STREAM_INSTALLLER) {
            taskProgress_.status = status == UpgradeStatus::DOWNLOAD_CANCEL ?
                UpgradeStatus::INSTALL_PAUSE : UpgradeStatus::INSTALL_FAIL;
        } else {
            taskProgress_.status = UpgradeStatus::UPDATE_FAIL;
        }
    }
    FIRMWARE_LOGI("HandleInstallResult status: %{public}d progress: %{public}d",
        taskProgress_.status, taskProgress_.percent);
    // 整体进度插入到 task 表
    FirmwareTaskOperator().UpdateProgressByTaskId(tasks_.taskId, taskProgress_.status, taskProgress_.percent);
    installCallbackInfo_.progress = taskProgress_;
    installCallbackInfo_.errorMessage.errorCode = errMsg.errorCode;
    installCallbackInfo_.errorMessage.errorMessage = errMsg.errorMessage;
    if (installCallback_.installCallback == nullptr) {
        FIRMWARE_LOGE("FirmwareInstallExecutor HandleInstallResult installCallback is null");
        return;
    }
    installCallback_.installCallback(installCallbackInfo_);
}

} // namespace UpdateService
} // namespace OHOS
