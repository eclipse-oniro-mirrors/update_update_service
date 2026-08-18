/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef UPDATE_SERVICE_PROGRESS_H
#define UPDATE_SERVICE_PROGRESS_H

#include <cstdint>
#include <string>

#include "upgrade_status.h"

namespace OHOS::UpdateService {
struct Progress {
    uint32_t percent = 0;
    UpgradeStatus status = UpgradeStatus::INIT;
    std::string endReason;

    Progress() = default;
    Progress(uint32_t percentValue, UpgradeStatus statusValue, const std::string &endReasonValue)
        : percent(percentValue), status(statusValue), endReason(endReasonValue){};

    Progress(const Progress &progress)
    {
        this->status = progress.status;
        this->percent = progress.percent;
        this->endReason = progress.endReason;
    };

    ~Progress() = default;

    Progress &operator = (const Progress &progress)
    {
        this->status = progress.status;
        this->percent = progress.percent;
        this->endReason = progress.endReason;
        return *this;
    }

    bool IsDownloadFinished() const
    {
        return ((status == UpgradeStatus::DOWNLOAD_PAUSE) || (status == UpgradeStatus::DOWNLOAD_CANCEL) ||
            (status == UpgradeStatus::DOWNLOAD_FAIL) || (status == UpgradeStatus::DOWNLOAD_SUCCESS));
    }
};
} // OHOS::UpdateService
#endif // UPDATE_SERVICE_PROGRESS_H
