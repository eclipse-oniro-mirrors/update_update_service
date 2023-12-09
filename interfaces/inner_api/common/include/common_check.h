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

#ifndef COMMON_CHECK_H
#define COMMON_CHECK_H

namespace OHOS::UpdateEngine {
#define RETURN_WHEN_REMOTE_NULL(remote) ENGINE_CHECK((remote) != nullptr, return INT_CALL_IPC_ERR, "Can not get remote")

#define IPC_RESULT_TO_CALL_RESULT(result)           \
    if ((result) == ERR_NONE) {                     \
        result = INT_CALL_SUCCESS;                  \
    } else if ((result) >= CALL_RESULT_OFFSET) {    \
        result = (result) - CALL_RESULT_OFFSET;     \
    } else {                                        \
        result = INT_CALL_IPC_ERR;                  \
    }
} // namespace OHOS::UpdateEngine
#endif // COMMON_CHECK_H