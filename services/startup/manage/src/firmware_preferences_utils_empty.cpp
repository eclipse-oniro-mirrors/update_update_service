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

#include "firmware_preferences_utils.h"

#include "update_log.h"

namespace OHOS {
namespace UpdateService {
FirmwarePreferencesUtil::FirmwarePreferencesUtil()
{
    FIRMWARE_LOGD("FirmwarePreferencesUtil empty constructor");
}

FirmwarePreferencesUtil::~FirmwarePreferencesUtil()
{
    FIRMWARE_LOGD("FirmwarePreferencesUtil empty deConstructor");
}

std::string FirmwarePreferencesUtil::GetPath()
{
    return "";
}

int64_t FirmwarePreferencesUtil::GetAutoUpgradeTime(int64_t defaultVal)
{
    return defaultVal;
}

void FirmwarePreferencesUtil::SaveAutoUpgradeTime(int64_t time) {}

bool FirmwarePreferencesUtil::GetAutoDownloadSwitch()
{
    return false;
}

void FirmwarePreferencesUtil::SaveAutoDownloadSwitch(bool value) {}

bool FirmwarePreferencesUtil::GetNightUpgradeSwitch()
{
    return false;
}

void FirmwarePreferencesUtil::SaveNightUpgradeSwitch(bool value) {}
} // namespace UpdateService
} // namespace OHOS
