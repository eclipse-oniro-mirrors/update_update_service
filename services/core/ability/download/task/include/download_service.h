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

#ifndef DOWNLOAD_SERVICE_H
#define DOWNLOAD_SERVICE_H

#include <atomic>

#include "download_thread.h"
#include "network_type.h"

namespace OHOS::UpdateService {
class DownloadService {
public:
    explicit DownloadService(const std::string &taskId,
        const std::map<std::string, std::shared_ptr<DownloadInfo>> &downloadInfoMap);
    virtual ~DownloadService();

    DownloadResult Start(OnDownloadCallback callback, DownloadError &downloadError);
    DownloadResult Pause(DownloadError &downloadError);
    DownloadResult Resume(NetType netType, DownloadError &downloadError);
    DownloadResult Cancel(DownloadError &downloadError, bool isDeleteFiles = true);

    void NetChangeToPause();
    void NetChangeToStart();
    void Recover(OnDownloadCallback callback);

    void RemoveDownloadTask(const std::string &taskId);
    void SetDisableCallbackFlag(bool flag);

protected:
    void CallbackUpdateDownloadProgress(DownloadCallbackInfo info);

private:
    virtual void StartDownloadThread(const DownloadInfo &downloadInfo);
    virtual void CancelUnexecutedTask() {}

    DownloadResult HandleRetry(const std::string &downloadId, DownloadEndReason endReason);

    void UpdateDownloadProgress(DownloadCallbackInfo info);
    void CallbackDownloadProgress(const DownloadCallbackInfo &info);
    void RemoveThread(const std::string &downloadId);

    bool IsNetSatisfied();

    void StopAllDownloadThread();
    void DeleteDownloadedFiles();
    void NotifyCancelCompleted();
    DownloadResult ClearPackage(const std::string &path);

    void NotifyPauseCompleted();
    void NotifyRecoverCompleted();

    std::shared_ptr<DownloadInfo> UpdateDownloadInfo(const DownloadCallbackInfo &info);
    void UpdateDownloadNetType(NetType netType);
    std::shared_ptr<DownloadInfo> GetDownloadInfo(const std::string &downloadId);

protected:
    ffrt::recursive_mutex downloadServiceMutex_;
    std::map<std::string, std::shared_ptr<DownloadThread>> threadMap_;

private:
    std::string taskId_;
    OnDownloadCallback onDownloadCallback_ = nullptr;
    std::map<std::string, std::shared_ptr<DownloadInfo>> serviceDownloadInfoMap_;
    std::atomic<bool> isStopCallback_ = false;
};
} // namespace OHOS::UpdateService
#endif // DOWNLOAD_SERVICE_H