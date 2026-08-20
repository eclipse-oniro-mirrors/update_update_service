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

#ifndef FIRMWARE_DOWNLOAD_MODE_H
#define FIRMWARE_DOWNLOAD_MODE_H

#include "business_error.h"
#include "device_adapter.h"
#include "download_options.h"
#include "error_message.h"
#include "firmware_common.h"
#include "firmware_download_data_processor.h"
#include "firmware_iexecute_mode.h"
#include "firmware_install_data_processor.h"
#include "firmware_task.h"
#include "progress.h"

namespace OHOS {
namespace UpdateService {
class FirmwareDownloadMode final : public FirmwareIExecuteMode {
public:
    FirmwareDownloadMode(const DownloadOptions &downloadOptions, BusinessError &businessError,
        OnExecuteFinishCallback onExecuteFinishCallback)
        : downloadOptions_(downloadOptions),
          businessError_(businessError),
          onExecuteFinishCallback_(onExecuteFinishCallback) {}

    ~FirmwareDownloadMode() = default;
    FirmwareStep GetNextStep(FirmwareStep step) final;
    void SetDownloadProgress(const Progress &progress) final;
    void SetDownloadCallbackInfo(const FirmwareDownloadCallbackInfo& callbackInfo) override;
    void SetDownloadEvent(const std::string &downloadTaskId, EventId eventId) override;
    DownloadOptions GetDownloadOptions() final;
    void HandleComplete() final;

private:
    FirmwareStep GetStepAfterInit();
    FirmwareStep GetStepAfterDownload();
    void GetTask();

private:
    FirmwareDownloadDataProcessor downloadDataProcessor_;
    DownloadOptions downloadOptions_;
    BusinessError &businessError_;
    OnExecuteFinishCallback onExecuteFinishCallback_;
    FirmwareTask tasks_;
};
} // namespace UpdateService
} // namespace OHOS
#endif // FIRMWARE_DOWNLOAD_MODE_H