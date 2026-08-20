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

#include "firmware_updater_install.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <unistd.h>

#include "updaterkits/updaterkits.h"

#include "dupdate_errno.h"
#include "firmware_constant.h"
#include "firmware_log.h"
#include "firmware_update_helper.h"

namespace OHOS {
namespace UpdateService {
bool UpdaterInstall::IsComponentLegal(const std::vector<FirmwareComponent> &componentList)
{
    return FirmwareUpdateHelper::IsUpgradePackagesReady(componentList);
}

bool UpdaterInstall::PerformInstall(const std::vector<FirmwareComponent> &componentList, UpgradeStatus &status)
{
    if (onInstallCallback_.onFirmwareStatus != nullptr) {
        onInstallCallback_.onFirmwareStatus(UpgradeStatus::UPDATING);
    }

    return DoUpdaterInstall(componentList) == OHOS_SUCCESS;
}

int32_t UpdaterInstall::DoUpdaterInstall(const std::vector<FirmwareComponent> &componentList)
{
    std::vector<std::string> installFiles;
    std::vector<std::pair<int32_t, std::string>> componentPaths;
    for (const auto &component : componentList) {
        componentPaths.emplace_back(component.blVersionType, component.spath);
    }
    sort(componentPaths.begin(), componentPaths.end());
    for (const auto &component : componentPaths) {
        installFiles.push_back(component.second);
    }
    sleep(1);
    int32_t ret = RebootAndInstallUpgradePackage("/misc", installFiles);
    if (ret != 0) {
        FIRMWARE_LOGE("RebootAndInstallUpgradePackage fail %{public}d", ret);
        errMsg_.errorCode = DUPDATE_ERR_UPDATE_REBOOT_FAIL;
        errMsg_.errorMessage = "updater install is failed, ret = " + std::to_string(ret);
        return OHOS_FAILURE;
    }
    return OHOS_SUCCESS;
}
} // namespace UpdateService
} // namespace OHOS
