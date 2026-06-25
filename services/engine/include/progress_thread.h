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

#ifndef UPDATER_THREAD_H
#define UPDATER_THREAD_H

#include <atomic>
#include <functional>
#include <thread>
#include <file_utils.h>
#include "curl/curl.h"

#include "progress.h"

namespace OHOS {
namespace UpdateService {
#define ENGINE_CHECK_NO_LOG(retCode, exper) \
    if (!(retCode)) {                     \
        exper;                            \
    }

constexpr uint32_t DOWNLOAD_FINISH_PERCENT = 100;
constexpr uint32_t DOWNLOAD_PERIOD_PERCENT = 1;
constexpr int32_t TIMEOUT_FOR_DOWNLOAD = 600;
constexpr int32_t TIMEOUT_FOR_STREAM_DOWNLOAD = 3600;
#ifndef UPDATER_UT
constexpr int32_t TIMEOUT_FOR_CONNECT = 10;
#else
constexpr int32_t TIMEOUT_FOR_CONNECT = 1;
#endif

class ProgressThread {
public:
    ProgressThread() = default;
    virtual ~ProgressThread();
    static std::atomic<bool> isNoNet_;
    static std::atomic<bool> isCancel_;
protected:
    int32_t StartProgress();
    void StopProgress();
    void QuitDownloadThread();
    void ExecuteThreadFunc();

    virtual bool ProcessThreadExecute() = 0;
    virtual void ProcessThreadExit() = 0;

private:
    std::thread *pDealThread_ { nullptr };
    std::mutex mutex_;
    std::condition_variable condition_;
    bool isWake_ = false;
    bool isExitThread_ = false;
};

class DownloadThread : public ProgressThread {
public:
    using ProgressCallback = std::function<void (const std::string serverUrl, const std::string &fileName,
        const Progress &progress)>;
    explicit DownloadThread(ProgressCallback callback) : ProgressThread(), callback_(callback) {}
    ~DownloadThread() override
    {
        ProgressThread::QuitDownloadThread();
    }

    int32_t StartDownload(const std::string &fileName, const std::string &url);
    void StopDownload();

    static size_t GetLocalFileLength(const std::string &fileName);
    static size_t WriteFunc(void *ptr, size_t size, size_t nmemb, const void *stream);
    static int32_t DownloadProgress(const void *localData,
        double dlTotal, double dlNow, double ulTotal, double ulNow);

    double GetPackageSize()
    {
        packageSize_ = GetLocalFileLength(downloadFileName_);
        return static_cast<double>(packageSize_);
    };

protected:
    bool ProcessThreadExecute() override;
    void ProcessThreadExit() override;
    int32_t DownloadCallback(uint32_t percent, UpgradeStatus status, const std::string &error);
    static FILE* FileOpen(const std::string &fileName, const std::string &mode);

private:
    Progress downloadProgress_ {};
    ProgressCallback callback_;
    CURL *downloadHandle_ { nullptr };
    FILE *downloadFile_ { nullptr };
    std::string serverUrl_;
    std::atomic<bool> exitDownload_ { false };
    size_t packageSize_ { 1 };
    std::string downloadFileName_;
    bool DealAbnormal(uint32_t percent);
};

// 缓冲区大小50KB
constexpr size_t BUFFER_SIZE = 50 * 1024;

class StreamProgressThread : public ProgressThread {
public:
    using ProgressCallback = std::function<void (const Progress &progress)>;
    explicit StreamProgressThread(ProgressCallback callback) : ProgressThread(), callback_(callback) {}
    ~StreamProgressThread() override
    {
        ProgressThread::QuitDownloadThread();
    }

    int32_t StartDownload(const std::string &url, const int64_t size, const int64_t recordPoint);
    void StopDownload();

    static size_t WriteFunc(uint8_t *ptr, size_t size, size_t nmemb, void *localData);
    static int32_t DownloadProgress(const void *localData,
        double dlTotal, double dlNow, double ulTotal, double ulNow);
        
protected:
    bool ProcessThreadExecute() override;
    void ProcessThreadExit() override;
    int32_t DownloadCallback(uint32_t percent, UpgradeStatus status, const std::string &error);

private:
    bool DealExitOrCancel();
    bool CheckFileSize();
    bool DownloadFile();
    static size_t HeaderCallback(char* buffer, size_t size, size_t nmemb, void* userp);
    
private:
    Progress downloadProgress_ {};
    ProgressCallback callback_;
    CURL *downloadHandle_ { nullptr };
    uint8_t buffer_[BUFFER_SIZE] = {0};
    size_t bufferPos_ = 0; // 缓冲区中的当前位置
    std::string serverUrl_;
    int64_t totalFileSize_ = 0; // 总文件大小
    int64_t downloadedSize_ = 0; // 已下载大小
    std::atomic<bool> exitDownload_ { false };
};
} // namespace UpdateService
} // namespace OHOS
#endif // UPDATER_THREAD_H
