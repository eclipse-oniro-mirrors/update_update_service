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

#ifndef FIRMWARE_AUTO_UPGRADE_MODE_H
#define FIRMWARE_AUTO_UPGRADE_MODE_H

#include "firmware_iexecute_mode.h"

#include "check_mode.h"
#include "constant.h"
#include "firmware_common.h"
#include "firmware_component.h"
#include "firmware_check_data_processor.h"
#include "firmware_download_data_processor.h"
#include "firmware_install_data_processor.h"
#include "firmware_task.h"
#include "firmware_update_adapter.h"

namespace OHOS::UpdateService {
class FirmwareAutoUpgradeMode final : public FirmwareIExecuteMode {
public:
    explicit FirmwareAutoUpgradeMode(const OnExecuteFinishCallback &onExecuteFinishCallback,
        CommonEventType autoUpgradeType = CommonEventType::AUTO_UPGRADE)
        : onExecuteFinishCallback_(onExecuteFinishCallback), autoUpgradeType_(autoUpgradeType)
    {}
    ~FirmwareAutoUpgradeMode() override = default;
    FirmwareStep GetNextStep(FirmwareStep step) override;
    void SetCheckResult(CheckStatus status, const Duration &duration,
        const std::vector<FirmwareComponent> &componentList, const CheckAndAuthInfo &checkAndAuthInfo) override;
    void SetInstallResult(const InstallCallbackInfo &installCallbackInfo) override;
    void HandleComplete() override;
    void SetDownloadCallbackInfo(const FirmwareDownloadCallbackInfo &callbackInfo) override;
    void SetDownloadEvent(const std::string &downloadTaskId, EventId eventId) override;

private:
    FirmwareStep GetStepAfterInit();
    FirmwareStep GetStepAfterCheck();
    FirmwareStep GetStepAfterDownload();
    CheckMode GetCheckMode(CommonEventType autoUpgradeType);
    CheckMode GetUpgradeCompleteCheckMode();
    FirmwareStep HandlePreDownloadStep();
    std::shared_ptr<FirmwareDownloadDataProcessor> GetDownloadDataProcessor();
    void GetTask();

    FirmwareCheckDataProcessor checkDataProcessor_;
    FirmwareInstallDataProcessor installStepDataProcessor_;
    OnExecuteFinishCallback onExecuteFinishCallback_;
    std::shared_ptr<FirmwareDownloadDataProcessor> downloadDataProcessor_ = nullptr;
    std::mutex downloadDataProcessorMutex_;
    FirmwareInstallDataProcessor installDataProcessor_;
    CommonEventType autoUpgradeType_ = CommonEventType::AUTO_UPGRADE;
    FirmwareTask task_;
};
} // namespace OHOS::UpdateService
#endif // FIRMWARE_AUTO_UPGRADE_MODE_H