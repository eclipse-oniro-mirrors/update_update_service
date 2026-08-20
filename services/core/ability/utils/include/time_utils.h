/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <chrono>
#include <cstdio>
#include <random>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <sstream>
#include <iomanip>

#include "update_log.h"

namespace OHOS {
namespace UpdateService {
class TimeUtils {
    static constexpr int64_t BOOT_COMPLETE_SIMULATE_DURATION = 60L; // 模拟开机广播：时间判断门限
    static constexpr int32_t ONE_THOUSAND_MS = 1000;

public:
    static int64_t GetTimestamp()
    {
        time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        return static_cast<int64_t>(currentTime);
    }

    static int64_t GetTimestampByMilliseconds()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    static std::string GetPrintTimeStr(int64_t time)
    {
        time_t printTime = static_cast<time_t>(time);
        tm *tmpTm = localtime(&printTime);
        if (tmpTm == NULL) {
            return "";
        }
        char timeStr[20];
        const size_t result = strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tmpTm);
        if (result == 0) {
            return ""; // 格式化失败
        }
        return std::string(timeStr);
    }

    static int64_t GetRandTime(int32_t minTime, int32_t maxTime)
    {
        // 随机 minTime ~ maxTime时间
        if (maxTime < minTime) {
            return minTime;
        }
        constexpr uint32_t randMin = 0;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(randMin, maxTime - minTime - 1);
        int64_t randomNumber = dis(gen);
        return randomNumber;
    }

    static int64_t GetSystemBootDuration()
    {
        int64_t bootSeconds =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count();
        int64_t currentTime = GetTimestamp();
        int64_t bootTime = currentTime - bootSeconds;
        ENGINE_LOGI("GetSystemBootDuration bootSeconeds is %{public}s, bootTime is %{public}s",
            std::to_string(bootSeconds).c_str(), GetPrintTimeStr(bootTime).c_str());
        return bootSeconds;
    }

    static bool IsInRebootDuration()
    {
        return GetSystemBootDuration() <= BOOT_COMPLETE_SIMULATE_DURATION;
    }
};
} // namespace UpdateService
} // namespace OHOS
#endif // TIME_UTILS_H