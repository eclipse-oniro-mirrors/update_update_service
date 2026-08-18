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

#ifndef DOWNLOAD_DATABASE_CALLBACK_H
#define DOWNLOAD_DATABASE_CALLBACK_H

#include "rdb_open_callback.h"

namespace OHOS::UpdateService {
class DownloadDatabaseCallback : public NativeRdb::RdbOpenCallback {
public:
    ~DownloadDatabaseCallback() override = default;

    int OnCreate(NativeRdb::RdbStore &rdbStore) override;
    int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override;
    int OnOpen(NativeRdb::RdbStore &rdbStore) override;
};
} // namespace OHOS::UpdateService
#endif // DOWNLOAD_DATABASE_CALLBACK_H