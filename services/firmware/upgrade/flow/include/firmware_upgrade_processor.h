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

#ifndef FIRMWARE_UPGRADE_PROCESSOR_H
#define FIRMWARE_UPGRADE_PROCESSOR_H

#include "firmware_task.h"

namespace OHOS::UpdateService {
class FirmwareUpgradeProcessor {
public:
    void ProcessBeforeUpgrade(const FirmwareTask &task);
    void ProcessAfterUpgrade(const FirmwareTask &task);

private:
    void HandleBeforeNightUpgrade(const FirmwareTask &task);
    void HandleAfterNightUpgrade(const FirmwareTask &task);
};
} // namespace OHOS::UpdateService

#endif // FIRMWARE_UPGRADE_PROCESSOR_H
