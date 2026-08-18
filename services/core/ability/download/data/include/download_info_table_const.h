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

#ifndef DOWNLOAD_INFO_TABLE_CONST_H
#define DOWNLOAD_INFO_TABLE_CONST_H

#include <string_view>

namespace OHOS::UpdateService {
constexpr std::string_view DOWNLOAD_TABLE_COMPONENT = "download";
constexpr std::string_view COLUMN_DOWNLOAD_ITEM_ID = "downloadId";
constexpr std::string_view COLUMN_DOWNLOAD_VERSION_ID = "versionId";
constexpr std::string_view COLUMN_DOWNLOAD_TASK_ID = "taskId";
constexpr std::string_view COLUMN_DOWNLOAD_URL = "url";
constexpr std::string_view COLUMN_DOWNLOAD_RESERVER_URL = "reserveUrl";
constexpr std::string_view COLUMN_DOWNLOAD_PATH = "path";
constexpr std::string_view COLUMN_DOWNLOAD_VERIFY_INFO = "veriftInfo";
constexpr std::string_view COLUMN_DOWNLOAD_POST_HEADER = "header";
constexpr std::string_view COLUMN_DOWNLOAD_POST_REQUEST_BODY = "requestBody";
constexpr std::string_view COLUMN_DOWNLOAD_STATUS = "status";
constexpr std::string_view COLUMN_DOWNLOAD_END_REASON = "endReason";
constexpr std::string_view COLUMN_DOWNLOAD_DIR_TYPE = "mode";
constexpr std::string_view COLUMN_DOWNLOAD_NET_TYPE = "netType";
constexpr std::string_view COLUMN_DOWNLOAD_REQUEST_METHOD = "method";
constexpr std::string_view COLUMN_DOWNLOAD_RETRY_TIMES = "retryTimes";
constexpr std::string_view COLUMN_DOWNLOAD_PACKAGE_SIZE = "packageSize";
constexpr std::string_view COLUMN_DOWNLOAD_DOWNLOADED_SIZE = "downloadedSize";
constexpr std::string_view COLUMN_DOWNLOAD_IS_NEED_AUTO_RESUME = "isNeedAutoResume";
constexpr std::string_view COLUMN_DOWNLOAD_START_TIME = "startTime";
constexpr std::string_view COLUMN_DOWNLOAD_END_TIME = "endTime";
constexpr std::string_view COLUMN_DOWNLOAD_EFFECTIVE_TIME = "effectiveTime";
constexpr std::string_view COLUMN_DOWNLOAD_IS_COMBINE_VERIFY_PROGRESS = "isCombineVerifyProgress";
constexpr std::string_view COLUMN_DOWNLOAD_VERIFIED_SIZE = "verifiedSize";
// 预留6个字段
constexpr std::string_view COLUMN_DOWNLOAD_EXTENDS1 = "extends1";
constexpr std::string_view COLUMN_DOWNLOAD_EXTENDS2 = "extends2";
constexpr std::string_view COLUMN_DOWNLOAD_EXTENDS3 = "extends3";
constexpr std::string_view COLUMN_DOWNLOAD_EXTENDS4 = "extends4";
constexpr std::string_view COLUMN_DOWNLOAD_EXTENDS5 = "extends5";
constexpr std::string_view COLUMN_DOWNLOAD_EXTENDS6 = "extends6";
} // namespace OHOS::UpdateService
#endif // DOWNLOAD_INFO_TABLE_CONST_H
