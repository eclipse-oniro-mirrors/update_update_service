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

#ifndef DOWNLOAD_TASK_MANAGER_H
#define DOWNLOAD_TASK_MANAGER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "safe_map.h"
#include "singleton.h"

#include "download_callback.h"
#include "download_task.h"

namespace OHOS::UpdateService {
class DownloadTaskManager : public DelayedSingleton<DownloadTaskManager> {
    DECLARE_DELAYED_SINGLETON(DownloadTaskManager);

public:
    std::string Start(std::vector<DownloadInfo> &downloadInfos, DownloadCallback callback, DownloadError &downloadError,
        DownloadDirType dirType);
    void Pause(std::string taskId, DownloadError &downloadError);
    void Resume(std::string taskId, NetType netType, DownloadCallback callback, DownloadError &downloadError);
    void Cancel(std::string taskId, DownloadError &downloadError, bool isDeleteFiles = true);
    void RemoveTask(std::string taskId, DownloadError &downloadError);
    ProgressInfo GetTaskProgress(std::string taskId, DownloadError &downloadError);
    std::string GetTaskId(const std::vector<DownloadInfo> &downloadInfos) const;

private:
    void Init();
    void RestoreDownloadTask();
    void OnNetChange(NetType netType);
    void DealCallBack(const std::string &taskId, ProgressInfo &progressInfo, DownloadCallback callback);
    bool IsDownloadComplete(const std::shared_ptr<DownloadTask> &downloadTask);
    std::vector<DownloadInfo> RemoveDuplicateVersionId(const std::vector<DownloadInfo> &downloadInfos);
    bool GetValidTask(const std::string &taskId, std::shared_ptr<DownloadTask> &outTask);

private:
    ffrt::recursive_mutex downloadTaskMapMutex_;
    SafeMap<std::string, std::shared_ptr<DownloadTask>> downloadTaskMap_;
    ffrt::queue delayDestroyTaskQueue_ = { "delayDestroyTaskQueue" };
};
} // namespace OHOS::UpdateService
#endif // DOWNLOAD_TASK_MANAGER_H