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

#ifndef DUPDATE_RANDOM_UTILS_H
#define DUPDATE_RANDOM_UTILS_H

#include <random>

namespace OHOS::UpdateService {
class RandomUtils {
public:
    static int64_t GetRand(int64_t min, int64_t max)
    {
        // 随机 min ~ max值
        if (max < min) {
            return min;
        }
        constexpr int64_t randMin = 0;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(randMin, max - min - 1);
        int64_t randomNumber = dis(gen);
        return min + randomNumber;
    }
};
} // namespace OHOS::UpdateService
#endif // DUPDATE_RANDOM_UTILS_H