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

#include "download_info_table.h"

#include "download_info_table_const.h"
#include "network_type.h"
#include "update_define.h"

namespace OHOS::UpdateService {
std::string DownloadInfoTable::GetTableName()
{
    return std::string(DOWNLOAD_TABLE_COMPONENT);
}

std::string DownloadInfoTable::GetTableCreateSql()
{
    return std::string("create table if not exists ")
        .append(std::string(DOWNLOAD_TABLE_COMPONENT) + " (")
        .append(std::string(COLUMN_ID) + " integer PRIMARY KEY autoincrement not null,")
        .append(std::string(COLUMN_DOWNLOAD_ITEM_ID) + " varchar(1024),")
        .append(std::string(COLUMN_DOWNLOAD_VERSION_ID) + " varchar(1024),")
        .append(std::string(COLUMN_DOWNLOAD_TASK_ID) + " varchar(128),")
        .append(std::string(COLUMN_DOWNLOAD_URL) + " varchar(1024),")
        .append(std::string(COLUMN_DOWNLOAD_RESERVER_URL) + " varchar(64),")
        .append(std::string(COLUMN_DOWNLOAD_PATH) + " varchar(256),")
        .append(std::string(COLUMN_DOWNLOAD_VERIFY_INFO) + " varchar(256),")
        .append(std::string(COLUMN_DOWNLOAD_POST_HEADER) + " varchar(10240),")
        .append(std::string(COLUMN_DOWNLOAD_POST_REQUEST_BODY) + " varchar(20480),")
        .append(std::string(COLUMN_DOWNLOAD_END_REASON) + " integer,")
        .append(std::string(COLUMN_DOWNLOAD_STATUS) + " integer,")
        .append(std::string(COLUMN_DOWNLOAD_DIR_TYPE) + " integer,")
        .append(std::string(COLUMN_DOWNLOAD_RETRY_TIMES) + " integer,")
        .append(std::string(COLUMN_DOWNLOAD_NET_TYPE) + " integer,")
        .append(std::string(COLUMN_DOWNLOAD_REQUEST_METHOD) + " integer,")
        .append(std::string(COLUMN_DOWNLOAD_PACKAGE_SIZE) + " bigint,")
        .append(std::string(COLUMN_DOWNLOAD_DOWNLOADED_SIZE) + " bigint,")
        .append(std::string(COLUMN_DOWNLOAD_IS_NEED_AUTO_RESUME) + " integer,")
        .append(std::string(COLUMN_DOWNLOAD_START_TIME) + " bigint,")
        .append(std::string(COLUMN_DOWNLOAD_END_TIME) + " bigint,")
        .append(std::string(COLUMN_DOWNLOAD_EFFECTIVE_TIME) + " bigint,")
        .append(std::string(COLUMN_DOWNLOAD_IS_COMBINE_VERIFY_PROGRESS) + " integer,")
        .append(std::string(COLUMN_DOWNLOAD_VERIFIED_SIZE) + " bigint,")
        .append(std::string(COLUMN_DOWNLOAD_EXTENDS1) + " varchar(256),")
        .append(std::string(COLUMN_DOWNLOAD_EXTENDS2) + " varchar(256),")
        .append(std::string(COLUMN_DOWNLOAD_EXTENDS3) + " varchar(256),")
        .append(std::string(COLUMN_DOWNLOAD_EXTENDS4) + " varchar(256),")
        .append(std::string(COLUMN_DOWNLOAD_EXTENDS5) + " varchar(256),")
        .append(std::string(COLUMN_DOWNLOAD_EXTENDS6) + " varchar(256)")
        .append(")");
}

void DownloadInfoTable::ParseDbValue(ResultSet *resultSet, DownloadInfo &value)
{
    GetColumnValue(resultSet, std::string(COLUMN_ID), value.id);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_ITEM_ID), value.downloadId);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_VERSION_ID), value.versionId);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_TASK_ID), value.taskId);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_URL), value.url);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_RESERVER_URL), value.reserveUrl);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_PATH), value.path);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_VERIFY_INFO), value.verifyInfo);

    std::string header;
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_POST_HEADER), header);
    value.header = Split2Map(header);

    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_POST_REQUEST_BODY), value.requestBody);
    int endReason = CAST_INT(DownloadEndReason::INIT);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_END_REASON), endReason);
    value.endReason = static_cast<DownloadEndReason>(endReason);

    int status = CAST_INT(DownloadStatus::DOWNLOADING);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_STATUS), status);
    value.status = static_cast<DownloadStatus>(status);

    int dirType = CAST_INT(DownloadDirType::UN_ENCRYPT_DIR);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_DIR_TYPE), dirType);
    value.dirType = static_cast<DownloadDirType>(dirType);

    int netType = CAST_INT(NetType::NO_NET);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_NET_TYPE), netType);
    value.netType = static_cast<NetType>(netType);

    int method = CAST_INT(RequestMethod::GET);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_REQUEST_METHOD), method);
    value.method = static_cast<RequestMethod>(method);

    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_RETRY_TIMES), value.retryTimes);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_PACKAGE_SIZE), value.packageSize);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_DOWNLOADED_SIZE), value.downloadedSize);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_IS_NEED_AUTO_RESUME), value.isNeedAutoResume);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_START_TIME), value.startTime);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_END_TIME), value.endTime);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_EFFECTIVE_TIME), value.effectiveTime);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_IS_COMBINE_VERIFY_PROGRESS), value.isCombineVerifyProgress);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_VERIFIED_SIZE), value.verifiedSize);
    GetColumnValue(resultSet, std::string(COLUMN_DOWNLOAD_EXTENDS1), value.isIgnoredFail);
}

void DownloadInfoTable::BuildDbValue(const DownloadInfo &value, NativeRdb::ValuesBucket &dbValue)
{
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_ITEM_ID), value.downloadId);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_VERSION_ID), value.versionId);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_TASK_ID), value.taskId);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_URL), value.url);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_RESERVER_URL), value.reserveUrl);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_PATH), value.path);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_VERIFY_INFO), value.verifyInfo);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_POST_HEADER), Concatenate(value.header));
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_POST_REQUEST_BODY), value.requestBody);

    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_END_REASON), static_cast<int>(value.endReason));
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_STATUS), static_cast<int>(value.status));
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_DIR_TYPE), static_cast<int>(value.dirType));
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_NET_TYPE), CAST_INT(value.netType));
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_REQUEST_METHOD), CAST_INT(value.method));
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_RETRY_TIMES), value.retryTimes);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_PACKAGE_SIZE), value.packageSize);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_DOWNLOADED_SIZE), value.downloadedSize);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_IS_NEED_AUTO_RESUME), value.isNeedAutoResume);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_START_TIME), value.startTime);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_END_TIME), value.endTime);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_EFFECTIVE_TIME), value.effectiveTime);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_IS_COMBINE_VERIFY_PROGRESS), value.isCombineVerifyProgress);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_VERIFIED_SIZE), value.verifiedSize);
    PutColumnValue(dbValue, std::string(COLUMN_DOWNLOAD_EXTENDS1), value.isIgnoredFail);
}

std::string DownloadInfoTable::Concatenate(const std::map<std::string, std::string> &inputMap)
{
    std::string str;
    for (const auto &[key, value] : inputMap) {
        str.append(key).append(":").append(value).append("|");
    }
    return str;
}

std::map<std::string, std::string> DownloadInfoTable::Split2Map(const std::string &str)
{
    std::map<std::string, std::string> split2Map;
    size_t start;
    size_t end = 0;
    while ((start = str.find_first_not_of("|", end)) != std::string::npos) {
        end = str.find("|", start);
        std::string element = str.substr(start, end - start);
        if (element.empty()) {
            continue;
        }
        size_t split = element.find(":");
        if (split == std::string::npos) {
            continue;
        }
        std::string key = element.substr(0, split);
        std::string value = element.substr(split + 1, element.length());
        split2Map.insert(make_pair(key, value));
    }
    return split2Map;
}
} // namespace OHOS::UpdateService