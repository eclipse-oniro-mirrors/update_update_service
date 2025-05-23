/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef UPDATE_SERVICE_CONFIG_INFO_H
#define UPDATE_SERVICE_CONFIG_INFO_H

#include <cstdint>
#include <string>

namespace OHOS::UpdateService {
struct ConfigInfo {
    std::string businessDomain;
    uint32_t abInstallTimeout = 1800; // 1800s
    uint32_t streamInstallTimeout = 3600; // 3600s
    std::string moduleLibPath;
};
} // OHOS::UpdateService
#endif //UPDATE_SERVICE_CONFIG_INFO_H
