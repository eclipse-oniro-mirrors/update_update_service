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

#include "download_database_callback.h"

#include <vector>

#include "download_info_table.h"
#include "update_log.h"

namespace OHOS::UpdateService {
int DownloadDatabaseCallback::OnCreate(NativeRdb::RdbStore &rdbStore)
{
    const auto &createSql = DownloadInfoTable().GetTableCreateSql();
    if (const int32_t ret = rdbStore.ExecuteSql(createSql); ret != NativeRdb::E_OK) {
        ENGINE_LOGE("DownloadDatabaseCallback OnCreate create table %{public}s, ret=%{public}d", createSql.c_str(),
            ret);
        return ret;
    }
    ENGINE_LOGI("DownloadDatabaseCallback OnCreate success");
    return NativeRdb::E_OK;
}

int DownloadDatabaseCallback::OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion)
{
    // do nothing
    return NativeRdb::E_OK;
}

int DownloadDatabaseCallback::OnOpen(NativeRdb::RdbStore &rdbStore)
{
    return NativeRdb::E_OK;
}
} // namespace OHOS::UpdateService