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

#ifndef FIRMWARE_SYS_INSTALLER_INSTALL_H
#define FIRMWARE_SYS_INSTALLER_INSTALL_H

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>

#include "firmware_component.h"
#include "firmware_install.h"

namespace OHOS {
namespace UpdateService {
using OnSysInstallerCallback = std::function<void(const InstallProgress &installProgress)>;

struct SysInstallerExecutorCallback {
    OnSysInstallerCallback onSysInstallerCallback;
};

class SysInstallerInstall final : public FirmwareInstall {
private:
    bool IsComponentLegal(const std::vector<FirmwareComponent> &componentList) final;
    bool PerformInstall(const std::vector<FirmwareComponent> &componentList, UpgradeStatus &status) final;
    int32_t StartUpdatePackageZip(const std::string &versionId, std::string &path);
    int32_t DoSysInstall(const FirmwareComponent &firmwareComponent);
    void InitInstallProgress();
    int32_t WaitInstallResult(const std::string &versionId);
    int32_t InitSysInstaller(const FirmwareComponent &firmwareComponent);
    int32_t SetupInstallCallback(FirmwareComponent firmwareComponent);

private:
    Progress sysInstallProgress_;
    std::mutex installMutex_;
    std::condition_variable installCond_;
    std::atomic<bool> resultReady_ { false };
};
} // namespace UpdateService
} // namespace OHOS
#endif // FIRMWARE_SYS_INSTALLER_INSTALL_H