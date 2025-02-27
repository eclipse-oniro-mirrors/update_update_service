/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef FIRMWARE_STREAM_INSTALLER_INSTALL_H
#define FIRMWARE_STREAM_INSTALLER_INSTALL_H

#include <string>
#include <condition_variable>
#include <mutex>
#include <chrono>

#include "firmware_component.h"
#include "firmware_install.h"
#include "progress_thread.h"

namespace OHOS {
namespace UpdateEngine {

constexpr int16_t MAX_RETRY_COUNT = 3; // 重试次数
constexpr int32_t RETRY_INTERVAL_TIME = 100 * 1000; // 重试间隔时间50ms

class StreamInstallerInstall final : public FirmwareInstall {
private:
    bool IsComponentLegal(const std::vector<FirmwareComponent> &componentList) final;
    bool PerformInstall(const std::vector<FirmwareComponent> &componentList, UpgradeStatus &status) final;
    int32_t StartInstallProcess(FirmwareComponent &component);
    int32_t DoSysInstall(const FirmwareComponent &component);
    bool InstallComponentWithRetry(const FirmwareComponent &component);
    int32_t WaitInstallResult();
    int32_t InitSysInstaller();
    int32_t SetInstallerCallback(FirmwareComponent &component);
    void NotifyCondition();

private:
    Progress downloadProgress_;
    Progress sysInstallProgress_;
    std::shared_ptr<StreamProgressThread> downloadThread_ = nullptr;
    std::mutex installMutex_;
    std::condition_variable installCond_;
};
} // namespace UpdateEngine
} // namespace OHOS
#endif // FIRMWARE_STREAM_INSTALLER_INSTALL_H