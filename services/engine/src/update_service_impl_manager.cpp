/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "update_service_impl_manager.h"

#include "update_log.h"
#include "update_service_impl_firmware.h"

using namespace std;

namespace OHOS {
namespace UpdateService {
UpdateServiceImplManager::UpdateServiceImplManager()
{
    ENGINE_LOGI("UpdateServiceImplManager");
}

UpdateServiceImplManager::~UpdateServiceImplManager()
{
    ENGINE_LOGI("~UpdateServiceImplManager");
}

sptr<IServiceOnlineUpdater> UpdateServiceImplManager::GetOnlineUpdater(const UpgradeInfo &info)
{
    std::lock_guard<std::mutex> lock(updateImplMapLock_);
    auto iter = updateImpMap_.find(info);
    if ((iter != updateImpMap_.end()) && (iter->second != nullptr)) {
        return iter->second;
    } else {
        sptr<IServiceOnlineUpdater> updateServiceImpl = nullptr;
        if (info.businessType.subType == BusinessSubType::FIRMWARE) {
            updateServiceImpl = new UpdateServiceImplFirmware();
        } else {
            ENGINE_LOGI("GetOnlineUpdater unknow business type");
        }
        if (updateServiceImpl != nullptr) {
            updateImpMap_.insert({info, updateServiceImpl});
        }
        return updateServiceImpl;
    }
}
} // namespace UpdateService
} // namespace OHOS