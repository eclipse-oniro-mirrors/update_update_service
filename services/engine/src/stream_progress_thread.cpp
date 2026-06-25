/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <file_utils.h>
#include <unistd.h>
#include <cinttypes>

#include "curl/curl.h"
#include "curl/easy.h"

#include "firmware_common.h"
#include "isys_installer.h"
#include "securec.h"
#include "sys_installer_kits_impl.h"
#include "update_define.h"
#include "update_log.h"

namespace OHOS {
namespace UpdateService {

int32_t StreamProgressThread::StartDownload(const std::string &url, const int64_t size, const int64_t recordPoint)
{
    ENGINE_LOGI("StartDownload url = %s", url.c_str());
    serverUrl_ = url;
    downloadedSize_ = recordPoint;
    totalFileSize_ = size;
    bufferPos_ = 0;
    isCancel_.store(false);
    exitDownload_ = false;
    (void)memset_s(buffer_, BUFFER_SIZE, 0, BUFFER_SIZE);
    curl_global_init(CURL_GLOBAL_ALL);
    return StartProgress();
}

void StreamProgressThread::StopDownload()
{
    ENGINE_LOGI("StopDownload");
    exitDownload_ = true;
    StopProgress();
    curl_global_cleanup();
}

bool StreamProgressThread::ProcessThreadExecute()
{
    ENGINE_LOGI("ProcessThreadExecute");
    downloadHandle_ = curl_easy_init();
    ENGINE_CHECK(downloadHandle_ != nullptr, ProcessThreadExit();
                 DownloadCallback(0, UpgradeStatus::DOWNLOAD_FAIL, "Failed to init curl");
                 return true, "Failed to init curl");
    if (!CheckFileSize()) {
        ProcessThreadExit();
        DownloadCallback(0, UpgradeStatus::DOWNLOAD_FAIL, std::to_string(CAST_INT(DownloadEndReason::CURL_ERROR)));
        return false;
    }

    if (!DownloadFile()) {
        ProcessThreadExit();
        DownloadCallback(0, UpgradeStatus::DOWNLOAD_CANCEL,
            std::to_string(CAST_INT(DownloadEndReason::CANCEL)));
    } else {
        ProcessThreadExit();
        DownloadCallback(DOWNLOAD_FINISH_PERCENT, UpgradeStatus::DOWNLOAD_SUCCESS, "");
    }

    return false;
}

bool StreamProgressThread::CheckFileSize()
{
    CURLcode res;
    curl_easy_setopt(downloadHandle_, CURLOPT_TIMEOUT, TIMEOUT_FOR_DOWNLOAD);
    curl_easy_setopt(downloadHandle_, CURLOPT_CONNECTTIMEOUT, TIMEOUT_FOR_CONNECT);
    curl_easy_setopt(downloadHandle_, CURLOPT_URL, serverUrl_.c_str());
    curl_easy_setopt(downloadHandle_, CURLOPT_NOBODY, 1L); // 只获取文件头部
    curl_easy_setopt(downloadHandle_, CURLOPT_HEADER, 1L); // 包含响应头部
    res = curl_easy_perform(downloadHandle_);
    if (res != CURLE_OK) {
        ENGINE_LOGE("Failed to curl_easy_perform res %s", curl_easy_strerror(res));
        return false;
    }



    // 获取文件大小
    double fileSize = 0;
    res = curl_easy_getinfo(downloadHandle_, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &fileSize);
    if (res != CURLE_OK || fileSize <= 0 || fileSize != totalFileSize_) {
        ENGINE_LOGE("Failed to curl_easy_getinfo res:%{public}s fileSize:%{public}f totalFileSize_:%{public}" PRId64 "",
                    curl_easy_strerror(res), fileSize, totalFileSize_);
        return false;
    }
    return true;
}

bool StreamProgressThread::DownloadFile()
{
    CURLcode res;
    curl_easy_setopt(downloadHandle_, CURLOPT_NOBODY, 0L); // 重置回去
    curl_easy_setopt(downloadHandle_, CURLOPT_HEADER, 0L); // 重置回去
    curl_easy_setopt(downloadHandle_, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(downloadHandle_, CURLOPT_WRITEFUNCTION, WriteFunc);

    ENGINE_LOGI("curl_easy_setopt downloadedSize_ %{public}" PRId64 "", downloadedSize_);
    if (downloadedSize_ > 0) {
        curl_easy_setopt(downloadHandle_, CURLOPT_RESUME_FROM_LARGE, static_cast<curl_off_t>(downloadedSize_));
    }
    curl_easy_setopt(downloadHandle_, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(downloadHandle_, CURLOPT_PROGRESSDATA, this);
    curl_easy_setopt(downloadHandle_, CURLOPT_PROGRESSFUNCTION, DownloadProgress);
    res = curl_easy_perform(downloadHandle_);
    if (res != CURLE_OK) {
        ENGINE_LOGE("Failed to download res:%{public}s", curl_easy_strerror(res));
        return false;
    } else {
        ENGINE_LOGI("Success to download");
        return true;
    }
}

void StreamProgressThread::ProcessThreadExit()
{
    ENGINE_LOGI("ProcessThreadExit");
    if (downloadHandle_ != nullptr) {
        curl_easy_cleanup(downloadHandle_);
    }
    downloadHandle_ = nullptr;
}

int32_t StreamProgressThread::DownloadCallback(uint32_t percent, UpgradeStatus status, const std::string &error)
{
    ENGINE_LOGI("DownloadCallback percent:%{public}d, status:%{public}d, error:%{public}s",
        percent, CAST_INT(status), error.c_str());

    downloadProgress_.endReason = error;
    downloadProgress_.percent = percent;
    downloadProgress_.status = status;
    if (callback_ != nullptr) {
        callback_(downloadProgress_);
    }
    return 0;
}

int32_t StreamProgressThread::DownloadProgress(const void *localData, double dlTotal, double dlNow, double ulTotal,
                                               double ulNow)
{
    auto engine = reinterpret_cast<StreamProgressThread *>(const_cast<void *>(localData));
    ENGINE_CHECK(engine != nullptr, return -1, "Can not find engine");
    unsigned int percent = 0;
    if (dlTotal > 0) {
        percent = dlNow / dlTotal * DOWNLOAD_FINISH_PERCENT;
        ENGINE_LOGI("StreamProgressThread DownloadProgress dlTotal:%{public}f,dlNow:%{public}f,ulTotal:%{public}f, "
            "ulNow:%{public}f,percent:%{public}d", dlTotal, dlNow, ulTotal, ulNow, percent);
    } else {
        ENGINE_LOGI("StreamProgressThread DownloadProgress dlTotal:%{public}f is less than 0", dlTotal);
        return 0;
    }
    return engine->DealExitOrCancel() ? -1 : 0;
}

size_t StreamProgressThread::WriteFunc(uint8_t *ptr, size_t size, size_t nmemb, void *localData)
{
    ENGINE_LOGI("StreamProgressThread WriteFunc");
    auto engine = reinterpret_cast<StreamProgressThread *>(const_cast<void *>(localData));
    ENGINE_CHECK(engine != nullptr, return -1, "Can not find engine");
    size_t totalSize = size * nmemb; // 本次收到的数据大小
    uint8_t *buffer = engine->buffer_;
    size_t& bufferPos = engine->bufferPos_;
    int64_t& totalFileSize = engine->totalFileSize_;
    int64_t& downloadedSize = engine->downloadedSize_;

    size_t processed = 0; // 已处理的数据量

    // 当还有未处理的数据时继续处理
    while (processed < totalSize) {
        size_t spaceLeft = BUFFER_SIZE - bufferPos - 1; // 当前缓冲区剩余空间
        size_t bytesToCopy = std::min(spaceLeft, totalSize - processed);

        // 将数据复制到缓冲区
        errno_t err = memcpy_s(buffer + bufferPos, BUFFER_SIZE - bufferPos, ptr + processed, bytesToCopy);
        if (err != 0) {
        ENGINE_LOGE("WriteFunc memcpy_s failed with error code: %{public}d", err);
            return -1;
        }
        bufferPos += bytesToCopy;
        processed += bytesToCopy;

        // 如果缓冲区已满，处理并清空
        if (bufferPos == (BUFFER_SIZE - 1) || (downloadedSize + processed) == totalFileSize) {
            ENGINE_LOGI("StreamProgressThread WriteFunc buffer full bufferPos:%{public}zu "
                        "(downloadedSize+processed):%{public}" PRId64 "",
                        bufferPos, downloadedSize + processed);
            #ifndef UPDATER_UT
            int32_t ret = SysInstaller::SysInstallerKitsImpl::GetInstance().ProcessStreamData(buffer, bufferPos);
            if (ret != 0) {
                ENGINE_LOGE("WriteFunc ProcessStreamData failed");
                return -1;
            }
            #endif
            // 清空缓冲区
            (void)memset_s(buffer, BUFFER_SIZE, 0, BUFFER_SIZE);
            bufferPos = 0;
        }
    }
    downloadedSize += totalSize;

    return totalSize;
}

bool StreamProgressThread::DealExitOrCancel()
{
    if (exitDownload_) {
        ENGINE_LOGI("DealExitOrCancel exit Download");
        return true;
    }
    if (isCancel_.load()) {
        ENGINE_LOGI("DealExitOrCancel install task cancel");
        return true;
    }
    return false;
}
} // namespace UpdateService
} // namespace OHOS
