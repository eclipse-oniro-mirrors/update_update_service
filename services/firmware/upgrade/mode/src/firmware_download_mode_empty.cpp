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

#include "firmware_download_mode.h"

#include "firmware_constant.h"
#include "firmware_log.h"

namespace OHOS {
namespace UpdateService {
FirmwareStep FirmwareDownloadMode::GetNextStep(FirmwareStep step)
{
    FIRMWARE_LOGI("GetNextStep %{public}d", static_cast<uint32_t>(step));
    return step;
}

FirmwareStep FirmwareDownloadMode::GetStepAfterInit()
{
    return FirmwareStep::DOWNLOAD_STEP;
}

FirmwareStep FirmwareDownloadMode::GetStepAfterDownload()
{
    FIRMWARE_LOGI("GetStepAfterDownload downloadOptions %{public}d", CAST_INT(downloadOptions_.order));
    return FirmwareStep::COMPLETE;
}

void FirmwareDownloadMode::GetTask()
{
    FIRMWARE_LOGI("FirmwareDownloadMode::GetTask");
    businessError_.Build(CallResult::FAIL, "no task!");
}

void FirmwareDownloadMode::HandleComplete()
{
    FIRMWARE_LOGI("FirmwareDownloadMode::HandleComplete");
    onExecuteFinishCallback_();
}

DownloadOptions FirmwareDownloadMode::GetDownloadOptions()
{
    return downloadOptions_;
}

void FirmwareDownloadMode::SetDownloadProgress(const Progress &progress)
{
    downloadDataProcessor_.SetDownloadProgress(progress);
}
} // namespace UpdateService
} // namespace OHOS