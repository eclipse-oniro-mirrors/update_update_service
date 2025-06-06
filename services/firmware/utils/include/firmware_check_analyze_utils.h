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

#ifndef FIRMWARE_CHECK_ANALYZE_UTILS_H
#define FIRMWARE_CHECK_ANALYZE_UTILS_H

#include <iostream>
#include <string>

#include "singleton.h"

#include "updateservice_json_utils.h"
#include "firmware_common.h"

namespace OHOS {
namespace UpdateService {
class FirmwareCheckAnalyzeUtils {
public:
    FirmwareCheckAnalyzeUtils() = default;
    ~FirmwareCheckAnalyzeUtils() = default;
    void DoAnalyze(const std::string &rawJson, std::vector<FirmwareComponent> &components, Duration &duration,
        CheckAndAuthInfo &checkAndAuthInfo);

private:
    bool IsLegalStatus(int32_t status);
    int32_t AnalyzeComponents(cJSON *root);
    int32_t ProcessCheckResults(cJSON *checkResults);
    int32_t ProcessDescriptInfo(cJSON *descriptInfo);
    int32_t AnalyzeBlVersionCheckResults(cJSON *root, BlCheckResponse &response);

private:
    std::vector<FirmwareComponent> components_;
};
} // namespace UpdateService
} // namespace OHOS
#endif // FIRMWARE_CHECK_ANALYZE_UTILS_H