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

#ifndef BASE_CALLBACK_UTILS_H
#define BASE_CALLBACK_UTILS_H

#include <string>

#include "business_sub_type.h"
#include "error_message.h"
#include "event_id.h"
#include "event_info.h"
#include "iupdate_callback.h"
#include "progress.h"
#include "upgrade_status.h"
#include "upgrade_info.h"
#include "version_component.h"

namespace OHOS {
namespace UpdateEngine {
class BaseCallbackUtils {
public:
    void NotifyEvent(const std::string &versionDigestInfo, EventId eventId, UpgradeStatus status,
        const ErrorMessage &errorMessage = {}, const std::vector<VersionComponent> &versionComponents = {});
    void ProgressCallback(const std::string &versionDigestInfo, const Progress &progress);

protected:
    virtual BusinessSubType GetBusinessSubType() = 0;

private:
    void NotifyToHap(EventInfo &info);
    void CallbackToHap(EventInfo &eventInfo);
    sptr<IUpdateCallback> GetUpgradeCallback(const UpgradeInfo &upgradeInfo);
};
} // namespace UpdateEngine
} // namespace OHOS
#endif // BASE_CALLBACK_UTILS_H