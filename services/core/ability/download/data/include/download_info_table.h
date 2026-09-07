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

#ifndef DOWNLOAD_INFO_TABLE_H
#define DOWNLOAD_INFO_TABLE_H

#include "download_common.h"

#include "values_bucket.h"

#include "itable.h"

namespace OHOS::UpdateService {
class DownloadInfoTable final : public ITable<DownloadInfo> {
public:
    DownloadInfoTable() = default;
    ~DownloadInfoTable() override = default;

    std::string GetTableName() override;
    std::string GetTableCreateSql() override;
    void ParseDbValue(ResultSet *resultSet, DownloadInfo &value) override;
    void BuildDbValue(const DownloadInfo &value, NativeRdb::ValuesBucket &dbValue) override;

private:
    std::string Concatenate(const std::map<std::string, std::string> &inputMap);
    std::map<std::string, std::string> Split2Map(const std::string &str);
};
} // namespace OHOS::UpdateService
#endif // DOWNLOAD_INFO_TABLE_H
