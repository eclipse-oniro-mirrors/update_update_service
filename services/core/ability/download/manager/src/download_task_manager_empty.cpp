/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "download_task_manager.h"

namespace OHOS::UpdateService {
DownloadTaskManager::DownloadTaskManager()
{
}

DownloadTaskManager::~DownloadTaskManager()
{
}

std::string DownloadTaskManager::Start(std::vector<DownloadInfo> &downloadInfos, DownloadCallback callback,
    DownloadError &downloadError, DownloadDirType dirType)
{
    return "";
}

void DownloadTaskManager::Pause(std::string taskId, DownloadError &downloadError)
{
}

void DownloadTaskManager::Resume(std::string taskId, NetType netType, DownloadCallback callback,
    DownloadError &downloadError)
{
}

void DownloadTaskManager::Cancel(std::string taskId, DownloadError &downloadError, bool isDeleteFiles)
{
}

void DownloadTaskManager::RemoveTask(std::string taskId, DownloadError &downloadError)
{
}

ProgressInfo DownloadTaskManager::GetTaskProgress(std::string taskId, DownloadError &downloadError)
{
    ProgressInfo progressInfo;
    return progressInfo;
}

std::string DownloadTaskManager::GetTaskId(const std::vector<DownloadInfo> &downloadInfos) const
{
    return "";
}
} // namespace OHOS::UpdateService
