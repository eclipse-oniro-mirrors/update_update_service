/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef FIRMWARE_DOWNLOAD_EXECUTOR_H
#define FIRMWARE_DOWNLOAD_EXECUTOR_H

#include "download_callback.h"
#include "download_common.h"
#include "download_options.h"
#include "firmware_component.h"
#include "firmware_iexecutor.h"
#include "firmware_task.h"

namespace OHOS::UpdateService {
class FirmwareDownloadExecutor : public std::enable_shared_from_this<FirmwareDownloadExecutor>,
    public FirmwareIExecutor {
public:
    FirmwareDownloadExecutor(const DownloadOptions &options, const FirmwareDownloadCallback &callback)
        : downloadOptions_(options), downloadCallback_(callback)
    {}
    ~FirmwareDownloadExecutor() override = default;

    void Execute() final;

private:
    void DoDownload();
    void GetTask();
    void PerformDownload();
    void PerformResumeDownload();
    bool VerifyDownloadPkg(const std::string &pkgName, Progress &progress);
    std::vector<DownloadInfo> BuildDownloadInfos() const;
    FirmwareDownloadProgress BuildFirmwareDownloadProgress(const Progress &progress,
        const DownloadProgress &downloadProgress);
    void CallbackProgress(std::string taskId, ProgressInfo progressInfo);
    std::string GetEndReason(UpgradeStatus status, DownloadEndReason reason);
    void CallbackDownloadFailProgress(DownloadError error);
    bool IsNeedResumeDownload(const ProgressInfo &progressInfo, DownloadError &downloadError);

private:
    DownloadOptions downloadOptions_;
    std::vector<FirmwareComponent> components_;
    FirmwareProgressCallback firmwareProgressCallback_;
    FirmwareTask tasks_;
    FirmwareDownloadCallback downloadCallback_;
};
}
#endif // FIRMWARE_DOWNLOAD_EXECUTOR_H