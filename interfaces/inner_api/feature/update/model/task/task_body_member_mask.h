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

#ifndef UPDATE_SERVICE_TASK_BODY_MEMBER_MASK_H
#define UPDATE_SERVICE_TASK_BODY_MEMBER_MASK_H

#include <map>

#include "event_id.h"

namespace OHOS::UpdateEngine {
enum TaskBodyMemberMask {
    VERSION_DIGEST_INFO = 0x00000001,
    UPGRADE_STATUS      = 0x00000010,
    SUB_STATUS          = 0x00000100,
    PROGRESS            = 0x00001000,
    INSTALL_MODE        = 0x00010000,
    ERROR_MESSAGE       = 0x00100000,
    VERSION_COMPONENT   = 0x01000000
};

const std::map<EventId, uint32_t> g_taskBodyTemplateMap = {
    { EventId::EVENT_TASK_RECEIVE,     VERSION_DIGEST_INFO },
    { EventId::EVENT_TASK_CANCEL,      VERSION_DIGEST_INFO },
    { EventId::EVENT_DOWNLOAD_WAIT,    VERSION_DIGEST_INFO | UPGRADE_STATUS | INSTALL_MODE },
    { EventId::EVENT_DOWNLOAD_START,   VERSION_DIGEST_INFO | INSTALL_MODE },
    { EventId::EVENT_DOWNLOAD_UPDATE,  VERSION_DIGEST_INFO | UPGRADE_STATUS | PROGRESS | INSTALL_MODE },
    { EventId::EVENT_DOWNLOAD_PAUSE,   VERSION_DIGEST_INFO | UPGRADE_STATUS | PROGRESS | INSTALL_MODE | ERROR_MESSAGE },
    { EventId::EVENT_DOWNLOAD_RESUME,  VERSION_DIGEST_INFO | UPGRADE_STATUS | PROGRESS | INSTALL_MODE },
    { EventId::EVENT_DOWNLOAD_SUCCESS, VERSION_DIGEST_INFO | INSTALL_MODE },
    { EventId::EVENT_DOWNLOAD_CANCEL,  VERSION_DIGEST_INFO | UPGRADE_STATUS },
    { EventId::EVENT_DOWNLOAD_FAIL,    VERSION_DIGEST_INFO | INSTALL_MODE | ERROR_MESSAGE },
    { EventId::EVENT_UPGRADE_WAIT,     VERSION_DIGEST_INFO | UPGRADE_STATUS | INSTALL_MODE | ERROR_MESSAGE },
    { EventId::EVENT_UPGRADE_START,    VERSION_DIGEST_INFO | UPGRADE_STATUS | INSTALL_MODE },
    { EventId::EVENT_UPGRADE_UPDATE,   VERSION_DIGEST_INFO | UPGRADE_STATUS | PROGRESS | INSTALL_MODE },
    { EventId::EVENT_APPLY_WAIT,       VERSION_DIGEST_INFO | UPGRADE_STATUS | ERROR_MESSAGE },
    { EventId::EVENT_APPLY_START,      VERSION_DIGEST_INFO },
    { EventId::EVENT_UPGRADE_SUCCESS,  VERSION_DIGEST_INFO | VERSION_COMPONENT },
    { EventId::EVENT_UPGRADE_FAIL,     VERSION_DIGEST_INFO | VERSION_COMPONENT | ERROR_MESSAGE },
    { EventId::EVENT_AUTH_START,       VERSION_DIGEST_INFO | VERSION_COMPONENT | UPGRADE_STATUS },
    { EventId::EVENT_AUTH_SUCCESS,     VERSION_DIGEST_INFO | VERSION_COMPONENT | UPGRADE_STATUS },
    { EventId::EVENT_INITIALIZE,       UPGRADE_STATUS },
};
} // namespace OHOS::UpdateEngine
#endif // UPDATE_SERVICE_TASK_BODY_MEMBER_MASK_H