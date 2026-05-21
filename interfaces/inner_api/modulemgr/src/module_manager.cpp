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

#include <climits>
#include <dlfcn.h>
#include <map>
#include <thread>

#include "../include/module_log.h"
#include "../include/module_manager.h"

namespace OHOS {
namespace UpdateService {
std::map<uint32_t, RequestFuncType> ModuleManager::onRemoteRequestFuncMap_;
std::map<std::string, LifeCycleFuncType> ModuleManager::onStartOnStopFuncMap_;
std::map<std::string, LifeCycleFuncReturnType> ModuleManager::onIdleFuncMap_;
std::map<std::string, LifeCycleFuncDumpType> ModuleManager::onDumpFuncMap_;

bool ModuleManager::isLoaded = false;

void ModuleManager::LoadModule(std::string libPath)
{
    std::string prefix = "/system/lib64/updateext";
    std::string modulePrefix = "/module_update/3006/lib64/updateext";
    std::string suffix = ".so";
    if (libPath.empty() || (libPath.length() < suffix.length())) {
        UTILS_LOGE("LoadModule lib path is empty or too short");
        return;
    }
    if ((libPath.substr(0, prefix.length()) != prefix &&
        libPath.substr(0, modulePrefix.length()) != modulePrefix) ||
        (libPath.substr(libPath.length() - suffix.length(), suffix.length()) != suffix)) {
            UTILS_LOGE("LoadModule lib path invalid");
            return;
    }
    UTILS_LOGD("LoadModule so path: %{public}s", libPath.c_str());
    if (dueModuleHandler == nullptr) {
        constexpr int32_t maxRetryTimes = 1;
        int32_t retryTimes = 0;
        char dealPath[PATH_MAX] = {};
        if (realpath(libPath.c_str(), dealPath) == nullptr) {
            UTILS_LOGE("soPath %{private}s is not exist or invalid", libPath.c_str());
            return;
        }
        do {
            dueModuleHandler = dlopen(dealPath, RTLD_LAZY);
            if (dueModuleHandler != nullptr) {
                isLoaded = true;
                break;
            }
            UTILS_LOGE("openSo path: %{public}s fail", libPath.c_str());
            retryTimes++;
            if (retryInterval_ > 0 && retryTimes <= maxRetryTimes) {
                std::this_thread::sleep_for(std::chrono::milliseconds(retryInterval_));
            }
        } while (retryTimes <= maxRetryTimes);
    } else {
        isLoaded = true;
        UTILS_LOGD("openSo ok");
    }
}

ModuleManager& ModuleManager::GetInstance()
{
    static ModuleManager moduleManager;
    return moduleManager;
}

void ModuleManager::HookFunc(std::vector<uint32_t> codes, RequestFuncType handleRemoteRequest)
{
    if (handleRemoteRequest == nullptr) {
        UTILS_LOGE("handleRemoteRequest is nullptr");
        return;
    }

    for (const uint32_t code : codes) {
        if (!IsMapFuncExist(code)) {
            UTILS_LOGI("add code %{public}d", code);
            onRemoteRequestFuncMap_.insert(std::make_pair(code, handleRemoteRequest));
        } else {
            UTILS_LOGI("code %{public}d already exist", code);
        }
    }
}

int32_t ModuleManager::HandleFunc(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (!IsMapFuncExist(code)) {
        UTILS_LOGI("code %{public}d not exist", code);
    } else {
        RequestFuncType func = onRemoteRequestFuncMap_[code];
        if (func == nullptr) {
            UTILS_LOGE("func for code %{public}d is nullptr", code);
            return -1;
        }
        UTILS_LOGI("code %{public}d called", code);
        return func(code, data, reply, option);
    }
    return 0;
}

ModuleManager::ModuleManager() {}

bool ModuleManager::IsModuleLoaded() const
{
    return isLoaded;
}

void ModuleManager::HookOnStartOnStopFunc(std::string phase, LifeCycleFuncType handleSAOnStartOnStop)
{
    if (handleSAOnStartOnStop == nullptr) {
        UTILS_LOGE("handleSAOnStartOnStop is nullptr");
        return;
    }

    if (onStartOnStopFuncMap_.find(phase) == onStartOnStopFuncMap_.end()) {
        UTILS_LOGI("add phase %{public}s", phase.c_str());
        onStartOnStopFuncMap_.insert(std::make_pair(phase, handleSAOnStartOnStop));
    } else {
        UTILS_LOGI("phase %{public}s exist", phase.c_str());
        onStartOnStopFuncMap_[phase] = handleSAOnStartOnStop;
    }
}

void ModuleManager::HandleOnStartOnStopFunc(std::string phase, const OHOS::SystemAbilityOnDemandReason &reason)
{
    if (onStartOnStopFuncMap_.find(phase) == onStartOnStopFuncMap_.end()) {
        UTILS_LOGI("phase %{public}s not exist", phase.c_str());
        return;
    }

    LifeCycleFuncType func = onStartOnStopFuncMap_[phase];
    if (func == nullptr) {
        UTILS_LOGE("func for phase is nullptr");
        return;
    }
    UTILS_LOGI("HandleOnStartOnStopFunc phase %{public}s exist", phase.c_str());
    func(reason);
}

void ModuleManager::HookOnIdleFunc(std::string phase, LifeCycleFuncReturnType handleSAOnIdle)
{
    if (handleSAOnIdle == nullptr) {
        UTILS_LOGE("handleSAOnIdle is nullptr");
        return;
    }

    if (onIdleFuncMap_.find(phase) == onIdleFuncMap_.end()) {
        UTILS_LOGI("add phase %{public}s", phase.c_str());
        onIdleFuncMap_.insert(std::make_pair(phase, handleSAOnIdle));
    } else {
        UTILS_LOGI("phase %{public}s already exist", phase.c_str());
        onIdleFuncMap_[phase] = handleSAOnIdle;
    }
}

int32_t ModuleManager::HandleOnIdleFunc(std::string phase, const OHOS::SystemAbilityOnDemandReason &reason)
{
    if (onIdleFuncMap_.find(phase) == onIdleFuncMap_.end()) {
        UTILS_LOGI("phase %{public}s not exist", phase.c_str());
    } else {
        LifeCycleFuncReturnType func = onIdleFuncMap_[phase];
        if (func == nullptr) {
            UTILS_LOGE("func for phase is nullptr");
            return -1;
        }
        UTILS_LOGI("phase %{public}s already exist", phase.c_str());
        return func(reason);
    }
    return 0;
}

void ModuleManager::HookDumpFunc(std::string phase, LifeCycleFuncDumpType handleSADump)
{
    if (handleSADump == nullptr) {
        UTILS_LOGE("handleSADump is nullptr");
        return;
    }

    if (onDumpFuncMap_.find(phase) == onDumpFuncMap_.end()) {
        UTILS_LOGI("add phase %{public}s", phase.c_str());
        onDumpFuncMap_.insert(std::make_pair(phase, handleSADump));
    } else {
        UTILS_LOGI("phase %{public}s already exist", phase.c_str());
        onDumpFuncMap_[phase] = handleSADump;
    }
}

int ModuleManager::HandleDumpFunc(std::string phase, int fd, const std::vector<std::u16string> &args)
{
    if (onDumpFuncMap_.find(phase) == onDumpFuncMap_.end()) {
        UTILS_LOGI("phase %{public}s not exist", phase.c_str());
    } else {
        LifeCycleFuncDumpType func = onDumpFuncMap_[phase];
        if (func == nullptr) {
            UTILS_LOGE("func is nullptr");
            return -1;
        }
        UTILS_LOGI("phase %{public}s already exist", phase.c_str());
        return func(fd, args);
    }
    return 0;
}

bool ModuleManager::IsMapFuncExist(uint32_t code) const
{
    return onRemoteRequestFuncMap_.count(code);
}
} // namespace UpdateService
} // namespace OHOS