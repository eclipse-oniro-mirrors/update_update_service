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

#include "download_info_operator.h"

#include "download_info_table_const.h"

namespace OHOS::UpdateService {
void DownloadInfoOperator::DeleteDownloadInfo(const DownloadInfo &downloadInfo)
{
    NativeRdb::RdbPredicates predicates(GetTableName());
    predicates.EqualTo(std::string(COLUMN_DOWNLOAD_ITEM_ID), downloadInfo.downloadId);
    Delete(predicates);
}

bool DownloadInfoOperator::UpdateProgressById(const DownloadInfo &downloadInfo)
{
    NativeRdb::ValuesBucket values;
    values.PutInt(std::string(COLUMN_DOWNLOAD_END_REASON), CAST_INT(downloadInfo.endReason));
    values.PutInt(std::string(COLUMN_DOWNLOAD_STATUS), CAST_INT(downloadInfo.status));
    values.PutLong(std::string(COLUMN_DOWNLOAD_DOWNLOADED_SIZE), downloadInfo.downloadedSize);
    values.PutLong(std::string(COLUMN_DOWNLOAD_START_TIME), downloadInfo.startTime);
    values.PutLong(std::string(COLUMN_DOWNLOAD_END_TIME), downloadInfo.endTime);
    values.PutLong(std::string(COLUMN_DOWNLOAD_EFFECTIVE_TIME), downloadInfo.effectiveTime);
    values.PutLong(std::string(COLUMN_DOWNLOAD_VERIFIED_SIZE), downloadInfo.verifiedSize);

    NativeRdb::RdbPredicates predicates(GetTableName());
    predicates.EqualTo(std::string(COLUMN_DOWNLOAD_ITEM_ID), downloadInfo.downloadId);
    return Update(values, predicates);
}

bool DownloadInfoOperator::UpdateStatusById(const DownloadInfo &downloadInfo)
{
    NativeRdb::ValuesBucket values;
    values.PutInt(std::string(COLUMN_DOWNLOAD_STATUS), CAST_INT(downloadInfo.status));

    NativeRdb::RdbPredicates predicates(GetTableName());
    predicates.EqualTo(std::string(COLUMN_DOWNLOAD_ITEM_ID), downloadInfo.downloadId);
    return Update(values, predicates);
}

bool DownloadInfoOperator::UpdateRetryTimesById(const DownloadInfo &downloadInfo)
{
    NativeRdb::ValuesBucket values;
    values.PutInt(std::string(COLUMN_DOWNLOAD_RETRY_TIMES), downloadInfo.retryTimes);

    NativeRdb::RdbPredicates predicates(GetTableName());
    predicates.EqualTo(std::string(COLUMN_DOWNLOAD_ITEM_ID), downloadInfo.downloadId);
    return Update(values, predicates);
}

bool DownloadInfoOperator::UpdateNetTypeById(const DownloadInfo &downloadInfo)
{
    NativeRdb::ValuesBucket values;
    values.PutInt(std::string(COLUMN_DOWNLOAD_NET_TYPE), CAST_INT(downloadInfo.netType));

    NativeRdb::RdbPredicates predicates(GetTableName());
    predicates.EqualTo(std::string(COLUMN_DOWNLOAD_ITEM_ID), downloadInfo.downloadId);
    return Update(values, predicates);
}

void DownloadInfoOperator::DeleteByTaskId(const std::string &taskId)
{
    NativeRdb::RdbPredicates predicates(GetTableName());
    predicates.EqualTo(std::string(COLUMN_DOWNLOAD_TASK_ID), taskId);
    Delete(predicates);
}
} // namespace OHOS::UpdateService
