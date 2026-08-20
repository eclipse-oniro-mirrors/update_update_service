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

#ifndef DOWNLOAD_DATABASE_H
#define DOWNLOAD_DATABASE_H

#include "singleton.h"

#include "download_database_callback.h"
#include "sqlite_db.h"

namespace OHOS::UpdateService {
inline const std::string DOWNLOAD_DB_NAME = "/download_sqlite.db";

class DownloadDatabase : public SqliteDb, public DelayedSingleton<DownloadDatabase> {
public:
    DownloadDatabase();
    ~DownloadDatabase() override = default;

    std::string GetDbName() override;
    int GetDbVersion() override;
    void InitDbStoreDir() override;
    std::string GetDbStoreDir() override;
    NativeRdb::RdbOpenCallback &GetDbOpenCallback() override;

private:
    std::string dbStoreDir_;
    DownloadDatabaseCallback callback_;
};
} // namespace OHOS::UpdateService
#endif // DOWNLOAD_DATABASE_H