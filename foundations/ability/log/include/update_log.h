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

#ifndef UPDATE_LOG_H
#define UPDATE_LOG_H

#include <cstdint>
#include <string>

#include <cinttypes>

#include "hilog/log.h"

namespace OHOS {
namespace UpdateService {
const std::string DEFAULT_LABEL = "%";
const std::string DEFAULT_FMT_LABEL = "%s";
const std::string PRIVATE_FMT_LABEL = "%{private}s";
const std::string PUBLIC_FMT_LABEL = "%{public}s";

static constexpr uint32_t  UPDATER_SA_DOMAIN_ID = 0xD002E00;
enum UpdaterModuleTags {
    UPDATE_SA_TAG = 0,
    UPDATE_KITS_TAG,
    UPDATE_FIRMWARE_TAG,
    UPDATE_MODULEMGR_TAG,
    UPDATE_MODULE_MAX
};

static constexpr OHOS::HiviewDFX::HiLogLabel UPDATE_LABEL[UPDATE_MODULE_MAX] = {
    { LOG_CORE, UPDATER_SA_DOMAIN_ID, "UPDATE_SERVICE_SA" },
    { LOG_CORE, UPDATER_SA_DOMAIN_ID, "UPDATE_SERVICE_KITS" },
    { LOG_CORE, UPDATER_SA_DOMAIN_ID, "UPDATE_SERVICE_FIRMWARE" },
    { LOG_CORE, UPDATER_SA_DOMAIN_ID, "UPDATE_SERVICE_MODULE_MGR" }
};

enum class UpdateLogLevel {
    UPDATE_DEBUG = 0,
    UPDATE_INFO,
    UPDATE_WARN,
    UPDATE_ERROR,
    UPDATE_FATAL
};

struct UpdateLogContent {
    HiviewDFX::HiLogLabel label;
    UpdateLogLevel level;
    std::string log;
    std::string args;
    std::string fileName;
    int32_t line = 0;

    UpdateLogContent BuildWithArgs(const std::string &argsInput) const
    {
        return {label, level, log, argsInput, fileName, line};
    };

    UpdateLogContent BuildWithFmtAndArgs(const std::string &logInput, const std::string &argsInput) const
    {
        return {label, level, logInput, argsInput, fileName, line};
    };
};

#ifdef UPDATE_SERVICE
constexpr int32_t UPDATE_LOG_TAG_ID = UPDATE_SA_TAG;
#else
constexpr int32_t UPDATE_LOG_TAG_ID = UPDATE_KITS_TAG;
#endif

class UpdateLog {
public:
    static bool JudgeLevel(const UpdateLogLevel &level);
    static void SetLogLevel(const UpdateLogLevel &level);
    static const UpdateLogLevel &GetLogLevel();
    static std::string GetBriefFileName(const std::string &file);
    static void PrintLongLog(const uint32_t subModuleTag, const UpdateLogContent &logContent);

private:
    static void PrintLog(const uint32_t subModuleTag, const UpdateLogContent &logContent);
    static void PrintSingleLine(const uint32_t subModuleTag, const UpdateLogContent &logContent);
    static std::pair<std::string, std::string> SplitLogByFmtLabel(const std::string &log, const std::string &fmtLabel);
    static std::string GetFmtLabel(const std::string &log);
    static int32_t FindSubStrCount(const std::string &str, const std::string &subStr);

private:
    static UpdateLogLevel level_;
};

#define R_FILENAME    (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

#define LONG_PRINT_HILOG(level, subtag, fmtlabel, fileName, line, fmt, ...) \
    if (fmtlabel == PUBLIC_FMT_LABEL) { \
        BASE_PRINT_LOG(level, subtag, fileName, line, "%{public}s", fmt, ##__VA_ARGS__); \
    } else if (fmtlabel == PRIVATE_FMT_LABEL) { \
        BASE_PRINT_LOG(level, subtag, fileName, line, "%{private}s", fmt, ##__VA_ARGS__); \
    } else { \
        BASE_PRINT_LOG(level, subtag, fileName, line, "%s", fmt, ##__VA_ARGS__); \
    }

#define BASE_PRINT_LOG(level, subtag, fileName, line, fmt, ...) \
    (void)HILOG_IMPL(LOG_CORE, level, UPDATE_LABEL[subtag].domain, UPDATE_LABEL[subtag].tag, \
    "[%{public}s(%{public}d)]" fmt, fileName, line, ##__VA_ARGS__)

#define PRINT_LOGE(subtag, fmt, ...) BASE_PRINT_LOG(LOG_ERROR, subtag, R_FILENAME, __LINE__, fmt, ##__VA_ARGS__)
#define PRINT_LOGI(subtag, fmt, ...) BASE_PRINT_LOG(LOG_INFO, subtag, R_FILENAME, __LINE__, fmt, ##__VA_ARGS__)
#define PRINT_LOGD(subtag, fmt, ...) BASE_PRINT_LOG(LOG_DEBUG, subtag, R_FILENAME, __LINE__, fmt, ##__VA_ARGS__)

#define ENGINE_LOGE(fmt, ...) PRINT_LOGE(UPDATE_LOG_TAG_ID, fmt, ##__VA_ARGS__)
#define ENGINE_LOGI(fmt, ...) PRINT_LOGI(UPDATE_LOG_TAG_ID, fmt, ##__VA_ARGS__)
#define ENGINE_LOGD(fmt, ...) PRINT_LOGD(UPDATE_LOG_TAG_ID, fmt, ##__VA_ARGS__)

#define PRINT_LONG_LOGD(subtag, label, fmt, args) UpdateLog::PrintLongLog(subtag, {label,    \
    UpdateLogLevel::UPDATE_DEBUG, std::string(fmt), std::string(args), std::string(__FILE__), __LINE__})
#define PRINT_LONG_LOGI(subtag, label, fmt, args) UpdateLog::PrintLongLog(subtag, {label,    \
    UpdateLogLevel::UPDATE_INFO, std::string(fmt), std::string(args), std::string(__FILE__), __LINE__})
#define PRINT_LONG_LOGE(subtag, label, fmt, args) UpdateLog::PrintLongLog(subtag, {label,    \
    UpdateLogLevel::UPDATE_ERROR, std::string(fmt), std::string(args), std::string(__FILE__), __LINE__})

#define ENGINE_LONG_LOGD(fmt, args) PRINT_LONG_LOGD(UPDATE_LOG_TAG_ID, UPDATE_LABEL[UPDATE_LOG_TAG_ID], fmt, args)
#define ENGINE_LONG_LOGI(fmt, args) PRINT_LONG_LOGI(UPDATE_LOG_TAG_ID, UPDATE_LABEL[UPDATE_LOG_TAG_ID], fmt, args)
#define ENGINE_LONG_LOGE(fmt, args) PRINT_LONG_LOGE(UPDATE_LOG_TAG_ID, UPDATE_LABEL[UPDATE_LOG_TAG_ID], fmt, args)
} // namespace UpdateService
} // namespace OHOS
#endif // UPDATE_LOG_H