/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "napi_common_utils.h"

#include "accesstoken_kit.h"
#include "access_token.h"
#include "ipc_skeleton.h"

#include "napi/native_common.h"
#include "node_api.h"

#include "tokenid_kit.h"
#include "update_define.h"

namespace OHOS::UpdateService {
constexpr int32_t STRING_MAX_LENGTH = 81920;

int32_t NapiCommonUtils::GetInt32(napi_env env, napi_value arg, const std::string &attrName, int32_t &intValue)
{
    bool result = false;
    napi_status status = napi_has_named_property(env, arg, attrName.c_str(), &result);
    if (result && (status == napi_ok)) {
        napi_value value = nullptr;
        napi_get_named_property(env, arg, attrName.c_str(), &value);
        napi_get_value_int32(env, value, &intValue);
        return CAST_INT(ClientStatus::CLIENT_SUCCESS);
    }
    return CAST_INT(ClientStatus::CLIENT_FAIL);
}

int32_t NapiCommonUtils::GetInt32(napi_env env, napi_value arg, int32_t &intValue)
{
    napi_valuetype valueType;
    napi_status status = napi_typeof(env, arg, &valueType);
    PARAM_CHECK(status == napi_ok && valueType == napi_number,
        return CAST_INT(ClientStatus::CLIENT_INVALID_TYPE), "invalid type");
    napi_get_value_int32(env, arg, &intValue);
    PARAM_CHECK(status == napi_ok, return CAST_INT(ClientStatus::CLIENT_INVALID_TYPE), "Error get GetInt32");
    return CAST_INT(ClientStatus::CLIENT_SUCCESS);
}

int32_t NapiCommonUtils::GetUInt32(napi_env env, napi_value arg, uint32_t &uintValue)
{
    napi_valuetype valueType;
    napi_status status = napi_typeof(env, arg, &valueType);
    PARAM_CHECK(status == napi_ok && valueType == napi_number,
        return CAST_INT(ClientStatus::CLIENT_INVALID_TYPE), "invalid type");
    napi_get_value_uint32(env, arg, &uintValue);
    PARAM_CHECK(status == napi_ok, return CAST_INT(ClientStatus::CLIENT_INVALID_TYPE), "Error get GetUInt32");
    return CAST_INT(ClientStatus::CLIENT_SUCCESS);
}

int32_t NapiCommonUtils::GetBool(napi_env env, napi_value arg, const std::string &attrName, bool &value)
{
    bool result = false;
    napi_status status = napi_has_named_property(env, arg, attrName.c_str(), &result);
    if (result && (status == napi_ok)) {
        napi_value obj = nullptr;
        napi_get_named_property(env, arg, attrName.c_str(), &obj);
        napi_get_value_bool(env, obj, &value);
        return CAST_INT(ClientStatus::CLIENT_SUCCESS);
    }
    return CAST_INT(ClientStatus::CLIENT_FAIL);
}

int32_t NapiCommonUtils::GetString(napi_env env, napi_value arg, const std::string &attrName, std::string &strValue)
{
    bool result = false;
    napi_status status = napi_has_named_property(env, arg, attrName.c_str(), &result);
    if (result && (status == napi_ok)) {
        napi_value value = nullptr;
        napi_get_named_property(env, arg, attrName.c_str(), &value);
        return GetString(env, value, strValue);
    }
    return CAST_INT(ClientStatus::CLIENT_FAIL);
}

int32_t NapiCommonUtils::GetString(napi_env env, napi_value arg, std::string &strValue)
{
    napi_valuetype valuetype;
    napi_status status = napi_typeof(env, arg, &valuetype);
    PARAM_CHECK(status == napi_ok, return CAST_INT(ClientStatus::CLIENT_FAIL), "Failed to napi_typeof");
    PARAM_CHECK(valuetype == napi_string, return CAST_INT(ClientStatus::CLIENT_INVALID_TYPE), "Invalid type");

    std::vector<char> buff(STRING_MAX_LENGTH);
    size_t copied;
    status = napi_get_value_string_utf8(env, arg, static_cast<char*>(buff.data()), STRING_MAX_LENGTH, &copied);
    PARAM_CHECK(status == napi_ok, return CAST_INT(ClientStatus::CLIENT_INVALID_TYPE), "Error get string");
    strValue.assign(buff.data(), copied);
    return CAST_INT(ClientStatus::CLIENT_SUCCESS);
}

int32_t NapiCommonUtils::SetString(napi_env env, napi_value arg, const std::string &attrName, const std::string &string)
{
    napi_value value = nullptr;
    napi_create_string_utf8(env, string.c_str(), string.length(), &value);
    napi_set_named_property(env, arg, attrName.c_str(), value);
    return CAST_INT(ClientStatus::CLIENT_SUCCESS);
}

int32_t NapiCommonUtils::SetUint32(napi_env env, napi_value arg, const std::string &attrName, uint32_t intValue)
{
    napi_value infoStatus = nullptr;
    napi_create_uint32(env, intValue, &infoStatus);
    napi_set_named_property(env, arg, attrName.c_str(), infoStatus);
    return CAST_INT(ClientStatus::CLIENT_SUCCESS);
}

int32_t NapiCommonUtils::SetInt32(napi_env env, napi_value arg, const std::string &attrName, int32_t intValue)
{
    napi_value infoStatus = nullptr;
    napi_create_int32(env, intValue, &infoStatus);
    napi_set_named_property(env, arg, attrName.c_str(), infoStatus);
    return CAST_INT(ClientStatus::CLIENT_SUCCESS);
}

int32_t NapiCommonUtils::SetInt64(napi_env env, napi_value arg, const std::string &attrName, int64_t intValue)
{
    napi_value infoStatus = nullptr;
    napi_create_int64(env, intValue, &infoStatus);
    napi_set_named_property(env, arg, attrName.c_str(), infoStatus);
    return CAST_INT(ClientStatus::CLIENT_SUCCESS);
}

int32_t NapiCommonUtils::SetBool(napi_env env, napi_value arg, const std::string &attrName, bool value)
{
    napi_value infoStatus = nullptr;
    napi_get_boolean(env, value, &infoStatus);
    napi_set_named_property(env, arg, attrName.c_str(), infoStatus);
    return CAST_INT(ClientStatus::CLIENT_SUCCESS);
}

ClientStatus NapiCommonUtils::IsTypeOf(napi_env env, napi_value arg, napi_valuetype type)
{
    napi_valuetype valueType;
    napi_status status = napi_typeof(env, arg, &valueType);
    PARAM_CHECK(status == napi_ok, return ClientStatus::CLIENT_FAIL, "Failed to napi_typeof");
    PARAM_CHECK(valueType == type, return ClientStatus::CLIENT_INVALID_TYPE, "Not same napi type");
    return ClientStatus::CLIENT_SUCCESS;
}

ClientStatus NapiCommonUtils::CreateReference(napi_env env, napi_value arg, uint32_t refcount, napi_ref &reference)
{
    napi_status status = napi_create_reference(env, arg, refcount, &reference);
    PARAM_CHECK(status == napi_ok, return ClientStatus::CLIENT_FAIL, "Failed to create reference");
    return ClientStatus::CLIENT_SUCCESS;
}

napi_value NapiCommonUtils::CreateUint32(napi_env env, uint32_t code)
{
    napi_value value = nullptr;
    if (napi_create_uint32(env, code, &value) != napi_ok) {
        return nullptr;
    }
    return value;
}

napi_value NapiCommonUtils::CreateStringUtf8(napi_env env, const std::string &str)
{
    napi_value value = nullptr;
    if (napi_create_string_utf8(env, str.c_str(), strlen(str.c_str()), &value) != napi_ok) {
        return nullptr;
    }
    return value;
}

void NapiCommonUtils::CreateProperty(napi_env env, napi_value exports, const std::string &name,
    const std::vector<std::pair<std::string, napi_value>> &properties)
{
    napi_value object = nullptr;
    napi_status status = napi_create_object(env, &object);
    if (status != napi_ok) {
        ENGINE_LOGE("CreateObject, napi_create_object fail");
    }
    if (object == nullptr) {
        return;
    }
    size_t size = properties.size();
    napi_property_descriptor descriptors[size];
    for (size_t pos = 0; pos < size; pos++) {
        if (properties[pos].first.empty()) {
            continue;
        }
        descriptors[pos] = DECLARE_NAPI_STATIC_PROPERTY(properties[pos].first.c_str(), properties[pos].second);
    }
    status = napi_define_properties(env, object, size, descriptors);
    if (status != napi_ok) {
        ENGINE_LOGE("DefineProperties, napi_define_properties fail");
    }

    status = napi_set_named_property(env, exports, name.c_str(), object);
    if (status != napi_ok) {
        ENGINE_LOGE("CreateProperty, napi_set_named_property fail");
    }
}

void NapiCommonUtils::NapiThrowParamError(
    napi_env env, std::vector<std::pair<std::string, std::string>> &paramInfos)
{
    BusinessError businessError;
    CallResult errCode = CallResult::PARAM_ERR;
    std::string errMsg = "BusinessError " + std::to_string(CAST_INT(errCode))
        .append(": Parameter error. The type of { ").append(GetParamNames(paramInfos)).append(" }")
        .append("must be { ").append(GetParamTypes(paramInfos)).append(" }.");
    businessError.Build(errCode, errMsg);
    napi_value msg = BuildThrowError(env, businessError);
    napi_status status = napi_throw(env, msg);
    PARAM_CHECK(status == napi_ok, return, "Failed to napi_throw %{public}d", CAST_INT(status));
}

void NapiCommonUtils::NapiThrowNotSystemAppError(napi_env env)
{
    BusinessError businessError;
    CallResult errCode = CallResult::NOT_SYSTEM_APP;
    std::string errMsg = "BusinessError " + std::to_string(CAST_INT(errCode))
        .append(": Permission verification failed. A non-system application calls a system API.");
    businessError.Build(errCode, errMsg);
    napi_value msg = BuildThrowError(env, businessError);
    napi_status status = napi_throw(env, msg);
    PARAM_CHECK(status == napi_ok, return, "Failed to napi_throw %{public}d", CAST_INT(status));
}

void NapiCommonUtils::NapiThrowUnSupportError(napi_env env)
{
    BusinessError businessError;
    CallResult errCode = CallResult::UN_SUPPORT;
    std::string errMsg = "BusinessError " + std::to_string(CAST_INT(errCode))
        .append(": Capability not supported. Failed to call the API due to limited device capabilities.");
    businessError.Build(errCode, errMsg);
    napi_value msg = BuildThrowError(env, businessError);
    napi_status status = napi_throw(env, msg);
    PARAM_CHECK(status == napi_ok, return, "Failed to napi_throw %{public}d", CAST_INT(status));
}

void NapiCommonUtils::NapiThrowPermissionError(napi_env env)
{
    BusinessError businessError;
    CallResult errCode = CallResult::APP_NOT_GRANTED;
    std::string errMsg = "BusinessError " + std::to_string(CAST_INT(errCode))
        .append(": Permission not granted.");
    businessError.Build(errCode, errMsg);
    napi_value msg = BuildThrowError(env, businessError);
    napi_status status = napi_throw(env, msg);
    PARAM_CHECK(status == napi_ok, return, "Failed to napi_throw %{public}d", CAST_INT(status));
}

bool NapiCommonUtils::IsPermissionGranted(const std::string &permission)
{
    OHOS::Security::AccessToken::AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    int verifyResult = Security::AccessToken::AccessTokenKit::VerifyAccessToken(callerToken, permission);
    bool isPermissionGranted = verifyResult == Security::AccessToken::PERMISSION_GRANTED;
    if (!isPermissionGranted) {
        ENGINE_LOGE("permission %{public}s not granted", permission.c_str());
    }
    return isPermissionGranted;
}

bool NapiCommonUtils::IsCallerValid()
{
    OHOS::Security::AccessToken::AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    auto callerTokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenType(callerToken);
    switch (callerTokenType) {
        case OHOS::Security::AccessToken::TypeATokenTypeEnum::TOKEN_HAP: {
            uint64_t callerFullTokenID = IPCSkeleton::GetCallingFullTokenID();
            // hap进程只允许系统应用调用
            return OHOS::Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(callerFullTokenID);
        }
        default:
            // 其他情况调用予以禁止
            return false;
    }
}

std::string NapiCommonUtils::GetParamNames(std::vector<std::pair<std::string, std::string>> &strVector)
{
    return ConvertVectorToStr(strVector, true);
}

std::string NapiCommonUtils::GetParamTypes(std::vector<std::pair<std::string, std::string>> &strVector)
{
    return ConvertVectorToStr(strVector, false);
}

std::string NapiCommonUtils::ConvertVectorToStr(std::vector<std::pair<std::string, std::string>> &strVector,
    bool isFirst)
{
    std::string strValue;
    for (auto &str : strVector) {
        if (!strValue.empty()) {
            strValue.append(", ");
        }
        if (isFirst) {
            strValue.append(str.first);
        } else {
            strValue.append(str.second);
        }
    }
    return strValue;
}

napi_value NapiCommonUtils::BuildThrowError(napi_env env, const BusinessError &businessError)
{
    napi_value message = nullptr;
    napi_create_string_utf8(env, businessError.message.c_str(), NAPI_AUTO_LENGTH, &message);
    napi_value error = nullptr;
    napi_status status = napi_create_error(env, nullptr, message, &error);
    PARAM_CHECK(status == napi_ok, return nullptr, "Failed to create napi_create_object %{public}d",
        CAST_INT(status));

    SetInt32(env, error, "code", ConvertToErrorCode(businessError.errorNum));
    SetString(env, error, "message", businessError.message);
    BuildErrorMessages(env, error, "data", businessError.data);
    return error;
}

int32_t NapiCommonUtils::BuildBusinessError(napi_env env, napi_value &obj, const BusinessError &businessError)
{
    if (businessError.errorNum == CallResult::SUCCESS) {
        // success, no need to set businessError
        return CAST_INT(ClientStatus::CLIENT_SUCCESS);
    }
    napi_status status = napi_create_object(env, &obj);
    PARAM_CHECK(status == napi_ok, return CAST_INT(ClientStatus::CLIENT_INVALID_TYPE),
        "Failed to create napi_create_object %{public}d", CAST_INT(status));

    SetString(env, obj, "message", businessError.message);
    SetInt32(env, obj, "code", ConvertToErrorCode(businessError.errorNum));
    BuildErrorMessages(env, obj, "data", businessError.data);
    return CAST_INT(ClientStatus::CLIENT_SUCCESS);
}

int32_t NapiCommonUtils::ConvertToErrorCode(CallResult callResult)
{
    if (IsCommonError(callResult) || callResult == CallResult::SUCCESS) {
        return CAST_INT(callResult);
    } else {
        return COMPONENT_ERR + CAST_INT(callResult);
    }
}

napi_value NapiCommonUtils::BuildCallFuncResult(napi_env env, const BusinessError &businessError)
{
    napi_value obj;
    napi_status status = napi_create_object(env, &obj);
    PARAM_CHECK(status == napi_ok, return obj, "Failed to create napi_create_object %{public}d", CAST_INT(status));
    napi_get_boolean(env, businessError.errorNum == CallResult::SUCCESS, &obj);
    return obj;
}

bool NapiCommonUtils::IsCommonError(CallResult callResult)
{
    return callResult == CallResult::UN_SUPPORT || callResult == CallResult::NOT_SYSTEM_APP ||
        callResult == CallResult::APP_NOT_GRANTED || callResult == CallResult::PARAM_ERR;
}

void NapiCommonUtils::BuildErrorMessages(napi_env env, napi_value &obj, const std::string &name,
    const std::vector<ErrorMessage> &errorMessages)
{
    size_t validErrorMsgCount = GetValidDataCount(errorMessages);
    if (validErrorMsgCount == 0) {
        return;
    }

    napi_value napiErrorMessages;
    napi_create_array_with_length(env, validErrorMsgCount, &napiErrorMessages);
    size_t index = 0;
    for (size_t i = 0; (i < errorMessages.size()) && (index < validErrorMsgCount); i++) {
        if (errorMessages[i].errorCode != 0) {
            napi_value napiErrorMessage;
            napi_create_object(env, &napiErrorMessage);
            SetInt32(env, napiErrorMessage, "errorCode", errorMessages[i].errorCode);
            SetString(env, napiErrorMessage, "errorMessage", errorMessages[i].errorMessage);
            napi_set_element(env, napiErrorMessages, index, napiErrorMessage);
            index++;
        }
    }
    napi_set_named_property(env, obj, name.c_str(), napiErrorMessages);
}

size_t NapiCommonUtils::GetValidDataCount(const std::vector<ErrorMessage> &list)
{
    size_t validDataCount = 0;
    for (const auto &errorMessage : list) {
        if (errorMessage.errorCode != 0) {
            validDataCount++;
        }
    }
    return validDataCount;
}

ClientStatus NapiCommonUtils::CheckNapiObjectType(napi_env env, const napi_value &arg)
{
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, arg, &type);
    PARAM_CHECK(status == napi_ok, return ClientStatus::CLIENT_INVALID_TYPE, "Invalid argc %{public}d",
        static_cast<int32_t>(status));
    PARAM_CHECK(type == napi_object, return ClientStatus::CLIENT_INVALID_TYPE, "Invalid argc %{public}d",
        static_cast<int32_t>(type))
    return ClientStatus::CLIENT_SUCCESS;
}
} // namespace OHOS::UpdateService