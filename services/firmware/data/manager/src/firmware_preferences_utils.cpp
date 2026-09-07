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

#include "constant.h"
#include "firmware_constant.h"
#include "firmware_log.h"

namespace OHOS {
namespace UpdateService {
FirmwarePreferencesUtil::FirmwarePreferencesUtil()
{
    FIRMWARE_LOGD("FirmwarePreferencesUtil");
}

FirmwarePreferencesUtil::~FirmwarePreferencesUtil()
{
    FIRMWARE_LOGD("~FirmwarePreferencesUtil");
}

std::string FirmwarePreferencesUtil::GetPath()
{
    return Constant::PREFERENCES_ROOT_PATH + "/update_firmware_sp.xml";
}

int64_t FirmwarePreferencesUtil::GetAutoUpgradeTime(int64_t defaultVal)
{
    return ObtainLong("auto_upgrade_time", defaultVal);
}

void FirmwarePreferencesUtil::SaveAutoUpgradeTime(int64_t time)
{
    SaveLong("auto_upgrade_time", time);
}

bool FirmwarePreferencesUtil::GetAutoDownloadSwitch()
{
    return ObtainBool(Firmware::AUTO_DOWNLOAD_SWITCH, false);
}

void FirmwarePreferencesUtil::SaveAutoDownloadSwitch(bool value)
{
    SaveBool(Firmware::AUTO_DOWNLOAD_SWITCH, value);
}

bool FirmwarePreferencesUtil::GetNightUpgradeSwitch()
{
    return ObtainBool("night_upgrade_switch", false);
}

void FirmwarePreferencesUtil::SaveNightUpgradeSwitch(bool value)
{
    SaveBool("night_upgrade_switch", value);
}
} // namespace UpdateService
} // namespace OHOS
