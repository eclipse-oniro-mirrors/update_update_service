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

#include "define_property.h"

#include "business_sub_type.h"
#include "business_vendor.h"
#include "call_result.h"
#include "component_type.h"
#include "description_format.h"
#include "description_type.h"
#include "effective_mode.h"
#include "factory_reset_scope.h"
#include "ota_mode.h"
#include "event_classify.h"
#include "event_id.h"
#include "napi_common_utils.h"
#include "network_type.h"
#include "order.h"
#include "string_utils.h"
#include "update_define.h"
#include "upgrade_action.h"
#include "upgrade_status.h"

#define DECLARE_ENUM_PROPERTY(item)      \
    {StringUtils::GetEnumValueString(#item), NapiCommonUtils::CreateUint32(env, CAST_UINT((item)))}

#define DECLARE_STRING_PROPERTY(item)    \
    {StringUtils::GetEnumValueString(#item), NapiCommonUtils::CreateStringUtf8(env, (item))}

#define DECLARE_CALL_RESULT_PROPERTY(item)      \
    {StringUtils::GetEnumValueString(#item),    \
        NapiCommonUtils::CreateUint32(env, CAST_UINT(NapiCommonUtils::ConvertToErrorCode(item)))}

namespace OHOS::UpdateService {
void DefineProperty::DefineProperties(napi_env env, napi_value exports)
{
    DefineCallResult(env, exports);
    DefineUpgradeStatus(env, exports);
    DefineComponentType(env, exports);
    DefineEffectiveMode(env, exports);
    DefineOtaType(env, exports);
    DefineNetType(env, exports);
    DefineOrder(env, exports);
    DefineEventClassify(env, exports);
    DefineBusinessSubType(env, exports);
    DefineDescriptionType(env, exports);
    DefineDescriptionFormat(env, exports);
    DefineEventId(env, exports);
    DefineUpgradeAction(env, exports);
    DefineBusinessVendor(env, exports);
    DefineFactoryResetScope(env, exports);
}

void DefineProperty::DefineCallResult(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> callResultList {
        DECLARE_CALL_RESULT_PROPERTY(CallResult::APP_NOT_GRANTED),
        DECLARE_CALL_RESULT_PROPERTY(CallResult::PARAM_ERR),
        DECLARE_CALL_RESULT_PROPERTY(CallResult::UN_SUPPORT),
        DECLARE_CALL_RESULT_PROPERTY(CallResult::SUCCESS),
        DECLARE_CALL_RESULT_PROPERTY(CallResult::FAIL),
        DECLARE_CALL_RESULT_PROPERTY(CallResult::DEV_UPG_INFO_ERR),
        DECLARE_CALL_RESULT_PROPERTY(CallResult::FORBIDDEN),
        DECLARE_CALL_RESULT_PROPERTY(CallResult::IPC_ERR),
        DECLARE_CALL_RESULT_PROPERTY(CallResult::TIME_OUT),
        DECLARE_CALL_RESULT_PROPERTY(CallResult::DB_ERROR),
        DECLARE_CALL_RESULT_PROPERTY(CallResult::IO_ERROR),
        DECLARE_CALL_RESULT_PROPERTY(CallResult::NET_ERROR)
    };
    NapiCommonUtils::CreateProperty(env, exports, "CallResult", callResultList);
}

void DefineProperty::DefineUpgradeStatus(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> upgradeStatusList = {
        DECLARE_ENUM_PROPERTY(UpgradeStatus::INIT),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::CHECKING_VERSION),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::CHECK_VERSION_FAIL),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::CHECK_VERSION_SUCCESS),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::DOWNLOADING),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::DOWNLOAD_PAUSE),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::DOWNLOAD_CANCEL),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::DOWNLOAD_FAIL),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::DOWNLOAD_SUCCESS),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::VERIFYING),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::VERIFY_FAIL),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::VERIFY_SUCCESS),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::PACKAGE_TRANSING),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::PACKAGE_TRANS_FAIL),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::PACKAGE_TRANS_SUCCESS),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::INSTALLING),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::INSTALL_FAIL),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::INSTALL_SUCCESS),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::INSTALL_PAUSE),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::UPDATING),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::UPDATE_FAIL),
        DECLARE_ENUM_PROPERTY(UpgradeStatus::UPDATE_SUCCESS)
    };
    NapiCommonUtils::CreateProperty(env, exports, "UpgradeStatus", upgradeStatusList);
}

void DefineProperty::DefineComponentType(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> componentTypeList = {
        DECLARE_ENUM_PROPERTY(ComponentType::OTA),
        DECLARE_ENUM_PROPERTY(ComponentType::PATCH),
        DECLARE_ENUM_PROPERTY(ComponentType::COTA),
        DECLARE_ENUM_PROPERTY(ComponentType::PARAM)
    };
    NapiCommonUtils::CreateProperty(env, exports, "ComponentType", componentTypeList);
}

void DefineProperty::DefineEffectiveMode(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> effectiveModeList = {
        DECLARE_ENUM_PROPERTY(EffectiveMode::COLD),
        DECLARE_ENUM_PROPERTY(EffectiveMode::LIVE),
        DECLARE_ENUM_PROPERTY(EffectiveMode::LIVE_AND_COLD)
    };
    NapiCommonUtils::CreateProperty(env, exports, "EffectiveMode", effectiveModeList);
}

void DefineProperty::DefineOtaType(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> otaTypeList = {
        DECLARE_ENUM_PROPERTY(OtaMode::REGULAR_OTA),
        DECLARE_ENUM_PROPERTY(OtaMode::STREAM_OTA),
        DECLARE_ENUM_PROPERTY(OtaMode::AB_REGULAR_OTA),
        DECLARE_ENUM_PROPERTY(OtaMode::AB_STREAM_OTA)
    };
    NapiCommonUtils::CreateProperty(env, exports, "OtaMode", otaTypeList);
}

void DefineProperty::DefineNetType(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> netTypeList = {
        DECLARE_ENUM_PROPERTY(NetType::CELLULAR),
        DECLARE_ENUM_PROPERTY(NetType::METERED_WIFI),
        DECLARE_ENUM_PROPERTY(NetType::NOT_METERED_WIFI),
        DECLARE_ENUM_PROPERTY(NetType::WIFI),
        DECLARE_ENUM_PROPERTY(NetType::CELLULAR_AND_WIFI)
    };
    NapiCommonUtils::CreateProperty(env, exports, "NetType", netTypeList);
}

void DefineProperty::DefineOrder(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> orderList = {
        DECLARE_ENUM_PROPERTY(Order::DOWNLOAD),
        DECLARE_ENUM_PROPERTY(Order::INSTALL),
        DECLARE_ENUM_PROPERTY(Order::DOWNLOAD_AND_INSTALL),
        DECLARE_ENUM_PROPERTY(Order::APPLY),
        DECLARE_ENUM_PROPERTY(Order::INSTALL_AND_APPLY)
    };
    NapiCommonUtils::CreateProperty(env, exports, "Order", orderList);
}

void DefineProperty::DefineEventClassify(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> eventClassifyList = {
        DECLARE_ENUM_PROPERTY(EventClassify::TASK),
        DECLARE_ENUM_PROPERTY(EventClassify::SYSTEM)
    };
    NapiCommonUtils::CreateProperty(env, exports, "EventClassify", eventClassifyList);
}

void DefineProperty::DefineBusinessSubType(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> businessSubTypeList = {
        DECLARE_ENUM_PROPERTY(BusinessSubType::FIRMWARE),
        DECLARE_ENUM_PROPERTY(BusinessSubType::PARAM),
        DECLARE_ENUM_PROPERTY(BusinessSubType::ROLLBACK)
    };
    NapiCommonUtils::CreateProperty(env, exports, "BusinessSubType", businessSubTypeList);
}

void DefineProperty::DefineDescriptionType(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> descriptionTypeList = {
        DECLARE_ENUM_PROPERTY(DescriptionType::CONTENT),
        DECLARE_ENUM_PROPERTY(DescriptionType::URI),
        DECLARE_ENUM_PROPERTY(DescriptionType::ID)
    };
    NapiCommonUtils::CreateProperty(env, exports, "DescriptionType", descriptionTypeList);
}

void DefineProperty::DefineDescriptionFormat(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> descriptionFormatList = {
        DECLARE_ENUM_PROPERTY(DescriptionFormat::STANDARD),
        DECLARE_ENUM_PROPERTY(DescriptionFormat::SIMPLIFIED)
    };
    NapiCommonUtils::CreateProperty(env, exports, "DescriptionFormat", descriptionFormatList);
}

void DefineProperty::DefineEventId(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> eventIdList = {
        DECLARE_ENUM_PROPERTY(EventId::EVENT_TASK_BASE),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_TASK_RECEIVE),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_TASK_CANCEL),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_DOWNLOAD_WAIT),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_DOWNLOAD_START),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_DOWNLOAD_UPDATE),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_DOWNLOAD_PAUSE),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_DOWNLOAD_RESUME),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_DOWNLOAD_SUCCESS),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_DOWNLOAD_FAIL),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_UPGRADE_WAIT),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_UPGRADE_START),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_UPGRADE_UPDATE),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_APPLY_WAIT),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_APPLY_START),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_UPGRADE_SUCCESS),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_UPGRADE_FAIL),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_AUTH_START),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_AUTH_SUCCESS),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_DOWNLOAD_CANCEL),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_INITIALIZE),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_TASK_CHANGE),
        DECLARE_ENUM_PROPERTY(EventId::EVENT_VERSION_INFO_CHANGE),
        DECLARE_ENUM_PROPERTY(EventId::SYSTEM_BASE),
        DECLARE_ENUM_PROPERTY(EventId::SYSTEM_BOOT_COMPLETE)
    };
    NapiCommonUtils::CreateProperty(env, exports, "EventId", eventIdList);
}

void DefineProperty::DefineUpgradeAction(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> upgradeActionList = {
        DECLARE_STRING_PROPERTY(UpgradeAction::UPGRADE),
        DECLARE_STRING_PROPERTY(UpgradeAction::RECOVERY),
        DECLARE_STRING_PROPERTY(UpgradeAction::ROLLBACK)
    };
    NapiCommonUtils::CreateProperty(env, exports, "UpgradeAction", upgradeActionList);
}

void DefineProperty::DefineBusinessVendor(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> businessVendorList = {
        DECLARE_STRING_PROPERTY(BusinessVendor::PUBLIC)
    };
    NapiCommonUtils::CreateProperty(env, exports, "BusinessVendor", businessVendorList);
}

void DefineProperty::DefineFactoryResetScope(napi_env env, napi_value exports)
{
    std::vector<std::pair<std::string, napi_value>> factoryResetScopeList = {
        DECLARE_ENUM_PROPERTY(FactoryResetScope::DATA),
        DECLARE_ENUM_PROPERTY(FactoryResetScope::DATA_AND_OS)
    };
    NapiCommonUtils::CreateProperty(env, exports, "FactoryResetScope", factoryResetScopeList);
}
} // namespace OHOS::UpdateService