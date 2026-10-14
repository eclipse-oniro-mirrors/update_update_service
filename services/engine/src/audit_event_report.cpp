/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "audit_event_report.h"

#include "call_result.h"
#include "cJSON.h"
#include "sg_collect_client.h"

namespace OHOS {
namespace UpdateService {
int32_t AuditEventReport::ReportResetAuditEvent(const std::string caller)
{
    constexpr int64_t eventId = 0x36000001;
    cJSON *callPkgJson = cJSON_CreateObject();
    if (callPkgJson == nullptr) {
        ENGINE_LOGE("callPkgJson create fail");
        return INT_CALL_FAIL;
    }
    if (!cJSON_AddNumberToObject(callPkgJson, "type", 0) ||
        !cJSON_AddNumberToObject(callPkgJson, "subType", -1) ||
        !cJSON_AddStringToObject(callPkgJson, "caller", caller.c_str()) ||
        !cJSON_AddStringToObject(callPkgJson, "objectInfo", "")) {
        ENGINE_LOGE("callPkgJson add fail");
        cJSON_Delete(callPkgJson);
        return INT_CALL_FAIL;
    }
    char *buff = cJSON_PrintUnformatted(callPkgJson);
    if (buff == nullptr) {
        ENGINE_LOGE("cJSON_PrintUnformatted fail");
        cJSON_Delete(callPkgJson);
        return INT_CALL_FAIL;
    }
    std::string reportContent(buff);
    cJSON_free(buff);
    cJSON_Delete(callPkgJson);
    if (reportContent.empty()) {
        ENGINE_LOGE("cJSON_PrintUnformatted fail, skip report");
        return INT_CALL_FAIL;
    }
    std::shared_ptr<OHOS::Security::SecurityGuard::EventInfo> eventInfo =
        std::make_shared<OHOS::Security::SecurityGuard::EventInfo>(eventId, "1.0", reportContent);
    int32_t ret = OHOS::Security::SecurityGuard::NativeDataCollectKit::ReportSecurityInfo(eventInfo);
    if (ret != 0) {
        ENGINE_LOGE("ReportSecurityInfo fail");
        return INT_CALL_FAIL;
    }
    return INT_CALL_SUCCESS;
}
} // namespace UpdateService
} // namespace OHOS