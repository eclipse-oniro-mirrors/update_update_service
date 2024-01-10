/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "progress_thread.h"

#include <unistd.h>
#include <file_utils.h>

#include "curl/curl.h"
#include "curl/easy.h"

#include "firmware_common.h"
#include "update_define.h"
#include "update_log.h"

namespace OHOS {
namespace UpdateEngine {
bool ProgressThread::isNoNet_ = false;
bool ProgressThread::isCancel_ = false;

ProgressThread::~ProgressThread() {}

void ProgressThread::QuitDownloadThread()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        isWake_ = true;
        isExitThread_ = true;
        condition_.notify_one();
    }
    if (pDealThread_ != nullptr) {
        pDealThread_->join();
        delete pDealThread_;
        pDealThread_ = nullptr;
    }
}

int32_t ProgressThread::StartProgress()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (pDealThread_ == nullptr) {
        pDealThread_ = new (std::nothrow)std::thread(&ProgressThread::ExecuteThreadFunc, this);
        ENGINE_CHECK(pDealThread_ != nullptr, return -1, "Failed to create thread");
    }
    ENGINE_LOGI("StartProgress");
    isWake_ = true;
    condition_.notify_one();
    return 0;
}

void ProgressThread::StopProgress()
{
    std::unique_lock<std::mutex> lock(mutex_);
    isWake_ = true;
    isExitThread_ = false;
    condition_.notify_one();
}

void ProgressThread::ExecuteThreadFunc()
{
    while (1) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while (!isWake_) {
                ENGINE_LOGI("ExecuteThreadFunc wait");
                condition_.wait(lock);
            }
            if (isExitThread_) {
                break;
            }
            isWake_ = false;
        }
        if (!ProcessThreadExecute()) {
            return;
        }
    }
    // thread exit and release resource
    ProcessThreadExit();
}

int32_t DownloadThread::StartDownload(const std::string &fileName, const std::string &url)
{
    ENGINE_LOGI("StartDownload downloadFileName_ %s, serverUrl_ = %s", downloadFileName_.c_str(), serverUrl_.c_str());
    downloadFileName_ = fileName;
    serverUrl_ = url;
    exitDownload_ = false;
    curl_global_init(CURL_GLOBAL_ALL);
    return StartProgress();
}

void DownloadThread::StopDownload()
{
    ENGINE_LOGI("StopDownload");
    exitDownload_ = true;
    StopProgress();
    curl_global_cleanup();
}

bool DownloadThread::ProcessThreadExecute()
{
    ENGINE_LOGI("ProcessThreadExecute");
    packageSize_ = GetLocalFileLength(downloadFileName_);
    ENGINE_LOGI("download  packageSize_: %zu ", packageSize_);
    bool findDot = (downloadFileName_.find("/.") != std::string::npos) ||
        (downloadFileName_.find("./") != std::string::npos);
    ENGINE_CHECK(!findDot,
        DownloadCallback(0, UpgradeStatus::DOWNLOAD_FAIL, "Failed to check file");
        return true, "Failed to check file %s", downloadFileName_.c_str());
    downloadFile_ = FileOpen(downloadFileName_, "ab+");
    ENGINE_CHECK(downloadFile_ != nullptr,
        DownloadCallback(0, UpgradeStatus::DOWNLOAD_FAIL, "Failed ot open file");
        return true, "Failed to open file %s", downloadFileName_.c_str());

    downloadHandle_ = curl_easy_init();
    ENGINE_CHECK(downloadHandle_ != nullptr,
        ProcessThreadExit();
        DownloadCallback(0, UpgradeStatus::DOWNLOAD_FAIL, "Failed to init curl");
        return true, "Failed to init curl");

    curl_easy_setopt(downloadHandle_, CURLOPT_TIMEOUT, TIMEOUT_FOR_DOWNLOAD);
    curl_easy_setopt(downloadHandle_, CURLOPT_CONNECTTIMEOUT, TIMEOUT_FOR_CONNECT);
    curl_easy_setopt(downloadHandle_, CURLOPT_URL, serverUrl_.c_str());
    curl_easy_setopt(downloadHandle_, CURLOPT_WRITEDATA, downloadFile_);
    curl_easy_setopt(downloadHandle_, CURLOPT_WRITEFUNCTION, WriteFunc);
    if (packageSize_ > 0) {
        curl_easy_setopt(downloadHandle_, CURLOPT_RESUME_FROM_LARGE, static_cast<curl_off_t>(packageSize_));
    }
    curl_easy_setopt(downloadHandle_, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(downloadHandle_, CURLOPT_PROGRESSDATA, this);
    curl_easy_setopt(downloadHandle_, CURLOPT_PROGRESSFUNCTION, DownloadProgress);
    CURLcode res = curl_easy_perform(downloadHandle_);
    if (res != CURLE_OK) {
        ProcessThreadExit();
        ENGINE_LOGI("Failed to download res %s", curl_easy_strerror(res));
        if (res != CURLE_ABORTED_BY_CALLBACK) { // cancel by user, do not callback
            DownloadCallback(0, UpgradeStatus::DOWNLOAD_FAIL,
                std::to_string(CAST_INT(DownloadEndReason::CURL_ERROR)));
        }
    } else {
        ProcessThreadExit();
        ENGINE_LOGI("Success to download");
        DownloadCallback(DOWNLOAD_FINISH_PERCENT, UpgradeStatus::DOWNLOAD_SUCCESS, "");
    }
    return false;
}

void DownloadThread::ProcessThreadExit()
{
    ENGINE_LOGI("ProcessThreadExit");
    if (downloadHandle_ != nullptr) {
        curl_easy_cleanup(downloadHandle_);
    }
    downloadHandle_ = nullptr;
    if (downloadFile_ != nullptr) {
        fclose(downloadFile_);
    }
    downloadFile_ = nullptr;
}

int32_t DownloadThread::DownloadCallback(uint32_t percent, UpgradeStatus status, const std::string &error)
{
    if (exitDownload_) {
        ENGINE_LOGI("StopDownloadCallback");
        return -1;
    }
    ENGINE_CHECK_NO_LOG(!DealAbnormal(percent),
        ENGINE_LOGI("DealAbnormal");
        return -1);
    if (downloadProgress_.status == status && downloadProgress_.percent == percent) {
        // 避免回调过于频繁
        return 0;
    }
    ENGINE_LOGI("DownloadCallback percent %d, status %d, exitDownload_ %d, error %s, downloadFileName_ %s",
        percent, CAST_INT(status), exitDownload_ ? 1 : 0,  error.c_str(), downloadFileName_.c_str());
    if (status == UpgradeStatus::DOWNLOAD_FAIL) {
        if (access(downloadFileName_.c_str(), 0) == 0) {
            unlink(downloadFileName_.c_str());
        }
    } else if (percent != DOWNLOAD_FINISH_PERCENT &&
               (percent < (downloadProgress_.percent + DOWNLOAD_PERIOD_PERCENT))) {
        return 0;
    }

    // wait until the download is complete, and then make a notification
    if (percent == DOWNLOAD_FINISH_PERCENT
        && status == UpgradeStatus::DOWNLOADING) {
        return 0;
    }
    downloadProgress_.endReason = error;
    downloadProgress_.percent = percent;
    downloadProgress_.status = status;
    if (callback_ != nullptr) {
        callback_(serverUrl_, downloadFileName_, downloadProgress_);
    }
    return 0;
}

int32_t DownloadThread::DownloadProgress(const void *localData,
    double dlTotal, double dlNow, double ulTotal, double ulNow)
{
    ENGINE_CHECK_NO_LOG(dlTotal > 0, return 0);
    auto engine = reinterpret_cast<DownloadThread*>(const_cast<void*>(localData));
    ENGINE_CHECK(engine != nullptr, return -1, "Can not find engine");
    double curr = engine->GetPackageSize();
    unsigned int percent = (dlNow + curr) / (curr + dlTotal) * DOWNLOAD_FINISH_PERCENT;
    return engine->DownloadCallback(percent, UpgradeStatus::DOWNLOADING, "");
}

size_t DownloadThread::WriteFunc(void *ptr, size_t size, size_t nmemb, const void *stream)
{
    return fwrite(ptr, size, nmemb, reinterpret_cast<FILE*>(const_cast<void*>(stream)));
}

size_t DownloadThread::GetLocalFileLength(const std::string &fileName)
{
    bool findDot = (fileName.find("/.") != std::string::npos) || (fileName.find("./") != std::string::npos);
    ENGINE_CHECK_NO_LOG(!findDot, return 0);

    FILE* fp = FileOpen(fileName, "r");
    ENGINE_CHECK_NO_LOG(fp != nullptr, return 0);
    int ret = fseek(fp, 0, SEEK_END);
    ENGINE_CHECK_NO_LOG(ret == 0, fclose(fp);
        return 0);
    size_t length = (size_t)ftell(fp);
    ret = fclose(fp);
    ENGINE_CHECK_NO_LOG(ret == 0, return 0);
    return length;
}

bool DownloadThread::DealAbnormal(uint32_t percent)
{
    bool dealResult = false;
    if (isNoNet_ || isCancel_) {
        ENGINE_LOGI("No network or user cancel");
        downloadProgress_.endReason = isNoNet_ ? std::to_string(CAST_INT(DownloadEndReason::NET_NOT_AVAILIABLE)) :
            std::to_string(CAST_INT(DownloadEndReason::CANCEL));
        downloadProgress_.percent = percent;
        downloadProgress_.status = isNoNet_ ? UpgradeStatus::DOWNLOAD_FAIL : UpgradeStatus::DOWNLOAD_CANCEL;
        if (isCancel_) {
            isCancel_ = false;
        }
        dealResult = true;
        if (callback_ != nullptr) {
            callback_(serverUrl_, downloadFileName_, downloadProgress_);
        }
    }
    return dealResult;
}

FILE* DownloadThread::FileOpen(const std::string &fileName, const std::string &mode)
{
    if (fileName.empty() || fileName.size() > PATH_MAX) {
        ENGINE_LOGI("DownloadThread file is empty or exceed path_max");
        return nullptr;
    }
    std::string fileDir = fileName;
    auto pos = fileDir.find_last_of("/");
    if (pos != std::string::npos) {
        fileDir.erase(pos + 1);
    } else {
        ENGINE_LOGI("DownloadThread file %{public}s, mode: %{public}s", fileName.c_str(), mode.c_str());
        return nullptr;
    }

    char *path = realpath(fileDir.c_str(), NULL);
    if (path == NULL) {
        ENGINE_LOGI("DownloadThread file %{public}s, mode: %{public}s", fileName.c_str(), mode.c_str());
        return nullptr;
    }
    free(path);
    FILE* fp = fopen(fileName.c_str(), mode.c_str());
    return fp;
}
} // namespace UpdateEngine
} // namespace OHOS
