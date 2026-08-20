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

#include "update_log.h"

namespace OHOS {
namespace UpdateService {
UpdateLogLevel UpdateLog::level_ = UpdateLogLevel::UPDATE_INFO;
constexpr int32_t COUNT_ONE = 1;
constexpr int32_t LONG_LOG_LEN = 900;

bool UpdateLog::JudgeLevel(const UpdateLogLevel &level)
{
    const UpdateLogLevel &curLevel = GetLogLevel();
    if (level <= curLevel) {
        return true;
    }
    return false;
}

void UpdateLog::SetLogLevel(const UpdateLogLevel &level)
{
    level_ = level;
}

const UpdateLogLevel &UpdateLog::GetLogLevel()
{
    return level_;
}

std::string UpdateLog::GetBriefFileName(const std::string &file)
{
    auto pos = file.find_last_of("/");
    if (pos != std::string::npos) {
        return file.substr(pos + 1);
    }
    pos = file.find_last_of("\\");
    if (pos != std::string::npos) {
        return file.substr(pos + 1);
    }
    return file;
}

void UpdateLog::PrintLongLog(const uint32_t subModuleTag, const UpdateLogContent &logContent)
{
    std::string fmtLabel = GetFmtLabel(logContent.log);
    std::pair<std::string, std::string> splitLogPair = SplitLogByFmtLabel(logContent.log, fmtLabel);

    PrintLog(subModuleTag, logContent.BuildWithFmtAndArgs(PUBLIC_FMT_LABEL, splitLogPair.first));     // log前缀不做打印控制
    PrintLog(subModuleTag, logContent.BuildWithFmtAndArgs(fmtLabel, logContent.args));                // args采用fmt进行控制
    PrintLog(subModuleTag, logContent.BuildWithFmtAndArgs(PUBLIC_FMT_LABEL, splitLogPair.second));    // log后缀不做打印控制
}

void UpdateLog::PrintLog(const uint32_t subModuleTag, const UpdateLogContent &logContent)
{
    int32_t printPos = 0;
    int32_t len = static_cast<int32_t>(logContent.args.length());
    while (printPos < len) {
        int32_t printLen = std::min(len - printPos, LONG_LOG_LEN);
        PrintSingleLine(subModuleTag, logContent.BuildWithArgs(logContent.args.substr(printPos, printLen)));
        printPos += printLen;
    }
}

void UpdateLog::PrintSingleLine(const uint32_t subModuleTag, const UpdateLogContent &logContent)
{
    // BASE_PRINT_LOG的第三个参数是hilog方法名，即hilogMethod
    std::string fmtLabel = GetFmtLabel(logContent.log);
    switch (logContent.level) {
        case UpdateLogLevel::UPDATE_DEBUG:
            LONG_PRINT_HILOG(LOG_DEBUG, subModuleTag, logContent.log,
                UpdateLog::GetBriefFileName(logContent.fileName).c_str(), logContent.line, logContent.args.c_str());
            break;
        case UpdateLogLevel::UPDATE_INFO:
            LONG_PRINT_HILOG(LOG_INFO, subModuleTag, logContent.log,
                UpdateLog::GetBriefFileName(logContent.fileName).c_str(), logContent.line, logContent.args.c_str());
            break;
        case UpdateLogLevel::UPDATE_ERROR:
            LONG_PRINT_HILOG(LOG_ERROR, subModuleTag, logContent.log,
                UpdateLog::GetBriefFileName(logContent.fileName).c_str(), logContent.line, logContent.args.c_str());
            break;
        default:
            break;
    }
}

std::pair<std::string, std::string> UpdateLog::SplitLogByFmtLabel(const std::string &log, const std::string &fmtLabel)
{
    if (fmtLabel.empty()) {
        // 如果log中没有%{public|private}s，则把log全部内容作为前缀字符串，后缀字符串为空
        return std::make_pair(log, "");
    }

    size_t labelPos = log.find(fmtLabel);
    if (labelPos == std::string::npos) {
        return std::make_pair(log, "");
    }

    //前缀是[0, labelPos), 后缀是[labelPos + len, end)
    size_t labelLen = fmtLabel.length();
    size_t suffixStart = labelPos + labelLen;
    return std::make_pair(log.substr(0, labelPos), log.substr(suffixStart));
}

std::string UpdateLog::GetFmtLabel(const std::string &log)
{
    if (FindSubStrCount(log, DEFAULT_LABEL) != COUNT_ONE) {
        // 如果log中%字符出现次数不为一个，说明log格式有误，返回空的fmtLabel
        return "";
    }
    if (FindSubStrCount(log, DEFAULT_FMT_LABEL) == COUNT_ONE) {
        return DEFAULT_FMT_LABEL;
    }
    if (FindSubStrCount(log, PRIVATE_FMT_LABEL) == COUNT_ONE) {
        return PRIVATE_FMT_LABEL;
    }
    if (FindSubStrCount(log, PUBLIC_FMT_LABEL) == COUNT_ONE) {
        return PUBLIC_FMT_LABEL;
    }
    return "";
}

int32_t UpdateLog::FindSubStrCount(const std::string &str, const std::string &subStr)
{
    int32_t count = 0;
    for (size_t pos = 0; (pos = str.find(subStr, pos)) != std::string::npos; pos++) {
        count++;
    }
    return count;
}
} // namespace UpdateService
} // namespace OHOS
