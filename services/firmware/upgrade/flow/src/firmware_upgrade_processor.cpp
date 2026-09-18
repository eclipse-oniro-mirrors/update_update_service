/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "firmware_upgrade_processor.h"

#include "bell_utils.h"
#include "firmware_log.h"
#include "firmware_preferences_utils.h"

namespace OHOS::UpdateService {
void FirmwareUpgradeProcessor::ProcessBeforeUpgrade(const FirmwareTask &task)
{
    // 升级前处理统一入口，后续新增处理项在此扩展。
    HandleBeforeNightUpgrade(task);
}

void FirmwareUpgradeProcessor::HandleBeforeNightUpgrade(const FirmwareTask &task)
{
    if (task.upgradeMode != UpgradeMode::NIGHT) {
        return;
    }

    if (!BellUtils::IsSupported()) {
        return;
    }

    bool isBellOn = BellUtils::GetBellStatus();
    auto preferencesUtil = DelayedSingleton<FirmwarePreferencesUtil>::GetInstance();
    if (!preferencesUtil->SaveBellStatus(isBellOn)) {
        FIRMWARE_LOGE("Save bell status failed");
        return;
    }

    if (isBellOn) {
        BellUtils::TurnOffBell();
        FIRMWARE_LOGI("Turn off boot animation sound before night upgrade");
    }
}

void FirmwareUpgradeProcessor::ProcessAfterUpgrade(const FirmwareTask &task)
{
    // 升级后处理统一入口，后续新增处理项在此扩展。
    HandleAfterNightUpgrade(task);
}

void FirmwareUpgradeProcessor::HandleAfterNightUpgrade(const FirmwareTask &task)
{
    if (task.upgradeMode != UpgradeMode::NIGHT) {
        return;
    }

    auto preferencesUtil = DelayedSingleton<FirmwarePreferencesUtil>::GetInstance();
    if (!preferencesUtil->GetBellStatus()) {
        return;
    }

    if (!BellUtils::IsSupported()) {
        FIRMWARE_LOGI("Restore boot animation sound ignored: bell is not supported");
        return;
    }

    BellUtils::TurnOnBell();
    if (!preferencesUtil->RemoveBellStatus()) {
        FIRMWARE_LOGE("Remove bell status failed");
    }
    FIRMWARE_LOGI("Restore boot animation sound after upgrade");
}
} // namespace OHOS::UpdateService
