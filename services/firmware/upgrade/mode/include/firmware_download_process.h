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

#ifndef FIRMWARE_DOWNLOAD_PROCESS_H
#define FIRMWARE_DOWNLOAD_PROCESS_H

#include "error_message.h"
#include "firmware_task_operator.h"

namespace OHOS::UpdateService {
class FirmwareDownloadProcess {
public:
    void DownloadSuccessProcess(const FirmwareTask &task, const ErrorMessage &errorMessage);
    void DownloadFailProcess(const FirmwareTask &task, const ErrorMessage &errorMessage);
    void DownloadCancelProcess(const FirmwareTask &task, const ErrorMessage &errorMessage, bool isDeleteFiles = true);
    void DownloadPauseProcess(const FirmwareTask &task, const ErrorMessage &errorMessage);
};
} // namespace OHOS::UpdateEngine
#endif // FIRMWARE_DOWNLOAD_PROCESS_H