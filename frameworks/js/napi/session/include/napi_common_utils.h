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

#ifndef NAPI_COMMON_UTILS_H
#define NAPI_COMMON_UTILS_H

#include <string>
#include <vector>

#include "js_native_api.h"
#include "js_native_api_types.h"

#include "business_error.h"
#include "call_result.h"
#include "error_message.h"
#include "napi_common_define.h"

namespace OHOS::UpdateService {
class NapiCommonUtils {
public:
    static int32_t GetBool(napi_env env, napi_value arg, const std::string &attrName, bool &value);
    static int32_t GetInt32(napi_env env, napi_value arg, const std::string &attrName, int32_t &intValue);
    static int32_t GetInt32(napi_env env, napi_value arg, int32_t &intValue);
    static int32_t GetUInt32(napi_env env, napi_value arg, uint32_t &uintValue);
    static int32_t GetString(napi_env env, napi_value arg, const std::string &attrName, std::string &strValue);
    static int32_t GetString(napi_env env, napi_value arg, std::string &strValue);

    static int32_t SetBool(napi_env env, napi_value arg, const std::string &attrName, bool value);
    static int32_t SetInt32(napi_env env, napi_value arg, const std::string &attrName, int32_t intValue);
    static int32_t SetInt64(napi_env env, napi_value arg, const std::string &attrName, int64_t intValue);
    static int32_t SetString(napi_env env, napi_value arg, const std::string &attrName, const std::string &string);
    static int32_t SetUint32(napi_env env, napi_value arg, const std::string &attrName, uint32_t intValue);

    static napi_value CreateUint32(napi_env env, uint32_t code);
    static napi_value CreateStringUtf8(napi_env env, const std::string &str);
    static ClientStatus CreateReference(napi_env env, napi_value arg, uint32_t refcount, napi_ref &reference);
    static void CreateProperty(napi_env env, napi_value exports, const std::string &name,
        const std::vector<std::pair<std::string, napi_value>> &properties);

    static ClientStatus IsTypeOf(napi_env env, napi_value arg, napi_valuetype type);
    static void NapiThrowParamError(napi_env env, std::vector<std::pair<std::string, std::string>> &paramInfos);
    static void NapiThrowNotSystemAppError(napi_env env);
    static void NapiThrowUnSupportError(napi_env env);
    static bool IsCallerValid();
    static bool IsPermissionGranted(const std::string &permission);
    static void NapiThrowPermissionError(napi_env env);
    static napi_value BuildThrowError(napi_env env, const BusinessError &businessError);
    static int32_t BuildBusinessError(napi_env env, napi_value &obj, const BusinessError &businessError);
    static int32_t ConvertToErrorCode(CallResult callResult);

    static napi_value BuildCallFuncResult(napi_env env, const BusinessError &businessError);

protected:
    static ClientStatus CheckNapiObjectType(napi_env env, const napi_value &arg);

private:
    static void BuildErrorMessages(napi_env env, napi_value &obj, const std::string &name,
        const std::vector<ErrorMessage> &errorMessages);
    static bool IsCommonError(CallResult callResult);
    static std::string GetParamNames(std::vector<std::pair<std::string, std::string>> &strVector);
    static std::string GetParamTypes(std::vector<std::pair<std::string, std::string>> &strVector);
    static std::string ConvertVectorToStr(std::vector<std::pair<std::string, std::string>> &strVector, bool isFirst);
    static size_t GetValidDataCount(const std::vector<ErrorMessage> &list);
};
} // namespace OHOS::UpdateService
#endif // NAPI_COMMON_UTILS_H