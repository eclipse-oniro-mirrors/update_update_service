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

#include "status_cache.h"

#include "constant.h"
#include "time_utils.h"
#include "update_log.h"

namespace OHOS {
namespace UpdateService {
bool StatusCache::IsChecking()
{
    if (lastCheckTime_ != -1 && abs(GetCurrentTime() - lastCheckTime_) > Constant::FIVE_MINUTES_SECONDS) {
        // 当前时间与上次检测时间间隔超过5分钟，允许再次触发搜包
        return false;
    }
    return isChecking_;
}

void StatusCache::SetIsChecking(bool isChecking)
{
    isChecking_ = isChecking;
    if (isChecking_) {
        lastCheckTime_ = GetCurrentTime();
    }
}

int64_t StatusCache::GetCurrentTime()
{
    if (timeUtilsProxy_ == nullptr) {
        ENGINE_LOGE("timeUtilsProxy is null");
        return 0;
    }
    return timeUtilsProxy_->GetTimestamp();
}

bool StatusCache::IsDownloadTriggered()
{
    if (lastDownloadTime_ == -1) {
        lastDownloadTime_ = TimeUtils::GetTimestampByMilliseconds();
        return false;
    }

    if (abs(TimeUtils::GetTimestampByMilliseconds() - lastDownloadTime_) < Constant::MILLESECONDS) {
        // 当前时间与上次下载时间间隔小于1秒钟，不允许重复触发下载
        ENGINE_LOGI("interval time within one seconds");
        return true;
    }
    lastDownloadTime_ = TimeUtils::GetTimestampByMilliseconds();
    return false;
}

bool StatusCache::IsUpgradeTriggered()
{
    if (lastUpgradeTime_ == -1) {
        lastUpgradeTime_ = TimeUtils::GetTimestampByMilliseconds();
        return false;
    }

    if (abs(TimeUtils::GetTimestampByMilliseconds() - lastUpgradeTime_) < Constant::MILLESECONDS) {
        // 当前时间与上次下载时间间隔小于1秒钟，不允许重复触发下载
        ENGINE_LOGI("interval time within one seconds");
        return true;
    }
    lastUpgradeTime_ = TimeUtils::GetTimestampByMilliseconds();
    return false;
}
} // namespace UpdateService
} // namespace OHOS
