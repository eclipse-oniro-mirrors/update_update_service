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

#ifndef FIRMWARE_DOWNLOAD_DATA_RPOCESSOR_H
#define FIRMWARE_DOWNLOAD_DATA_RPOCESSOR_H

#include "event_id.h"
#include "firmware_common.h"
#include "firmware_component_operator.h"
#include "firmware_task.h"
#include "firmware_task_operator.h"
#include "progress.h"

namespace OHOS {
namespace UpdateService {
class FirmwareDownloadDataProcessor {
public:
    FirmwareDownloadDataProcessor();
    ~FirmwareDownloadDataProcessor();

    void SetDownloadProgress(const Progress &progress);
    Progress GetDownloadProgress();
    bool IsDownloadSuccess();
    bool IsSpaceEnough(int64_t &requireTotalSize);
    void SetDownloadCallbackInfo(const FirmwareDownloadCallbackInfo &callbackInfo);
    void SetDownloadEvent(const std::string &downloadTaskId, EventId eventId);

private:
    void GetTask();

private:
    FirmwareTask tasks_;
    Progress downloadProgress_;

    FirmwareComponentOperator componentOperator_;
    FirmwareTaskOperator taskOperator_;
};
} // namespace UpdateService
} // namespace OHOS
#endif // FIRMWARE_DOWNLOAD_DATA_RPOCESSOR_H