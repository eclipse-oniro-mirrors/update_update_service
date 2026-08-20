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

#include "file_utils.h"

#include <cinttypes>
#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <vector>

#include "constant.h"
#include "update_log.h"

namespace OHOS {
namespace UpdateService {
std::map<std::string, int32_t> FileUtils::baseDirMap_;

int64_t FileUtils::RetrieveFileSize(const std::string &fileName)
{
    std::error_code errorCode;
    int64_t fileSize = static_cast<int64_t>(std::filesystem::file_size(fileName, errorCode));
    if (errorCode.operator bool()) {
        ENGINE_LOGE("get file size error, file = %{public}s", fileName.c_str());
        return 0;
    }
    return fileSize;
}

bool FileUtils::IsFileExist(const std::string &fileName)
{
    std::ifstream f(fileName.c_str());
    return f.good();
}

int64_t FileUtils::GetFileSize(const std::string &fileName)
{
    std::error_code errorCode;
    int64_t fileSize = static_cast<int64_t>(std::filesystem::file_size(fileName, errorCode));
    if (errorCode.operator bool()) {
        ENGINE_LOGE("get file size error, file = %{public}s", fileName.c_str());
        return 0;
    }
    return fileSize;
}

bool FileUtils::IsSpaceEnough(const std::string &filePath, const int64_t requiredSpace)
{
    uint64_t freeSpace = 0;
    struct statfs diskStatfs;
    int ret = statfs(filePath.c_str(), &diskStatfs);
    if (ret >= 0) {
        freeSpace = (uint64_t)diskStatfs.f_bsize * (uint64_t)diskStatfs.f_bavail;
    } else {
        ENGINE_LOGE("statfs fail, error code = %{public}d", ret);
    }
    ENGINE_LOGI("free space=%{public}" PRIu64 ", required space=%{public}" PRIu64 "", freeSpace,
        static_cast<uint64_t>(requiredSpace));
    return freeSpace >= static_cast<uint64_t>(requiredSpace);
}

bool FileUtils::SaveDataToFile(const std::string &filePath, const std::string &data)
{
    std::ofstream os;
    os.open(filePath, std::ios::trunc);
    if (os.is_open()) {
        ENGINE_LOGI("SaveDataToFile success, file = %{public}s", filePath.c_str());
        os << data;
        os.close();
        return true;
    }
    ENGINE_LOGE("SaveDataToFile fail, file = %{public}s", filePath.c_str());
    os.close();
    return false;
}

void FileUtils::DeleteFile(const std::string &rootPath, bool isDeleteRootDir)
{
    if (!IsFileExist(rootPath)) {
        ENGINE_LOGE("dir[%{public}s] is not exist", rootPath.c_str());
        return;
    }

    auto myPath = std::filesystem::path(rootPath);
    if (isDeleteRootDir) {
        RemoveAll(myPath);
        return;
    }

    for (auto const & dirEntry : std::filesystem::directory_iterator{ myPath }) {
        RemoveAll(dirEntry.path());
    }
}

void FileUtils::RemoveAll(const std::filesystem::path &path)
{
    std::error_code errorCode;
    std::filesystem::remove_all(path, errorCode);
    if (errorCode.operator bool()) {
        ENGINE_LOGE("remove dir[%{public}s] fail, error message : %{public}s", path.c_str(),
            errorCode.message().c_str());
    } else {
        ENGINE_LOGI("remove dir[%{public}s] success", path.c_str());
    }
}

// 此函数功能为创建多层目录，支持a/b和a/b/这两种形式，最终的结果为创建a目录以及b目录
bool FileUtils::CreateMultiDirWithPermission(const std::string &fileDir, int32_t permission)
{
    std::string curDir = GetCurrentDir(fileDir);
    if (IsFileExist(curDir)) {
        return true;
    }
    std::stack<std::string> dirStack;
    do {
        dirStack.push(curDir);
        curDir = GetParentDir(curDir);
    } while (!curDir.empty() && !IsFileExist(curDir));

    while (!dirStack.empty()) {
        if (!CreatDirWithPermission(dirStack.top(), baseDirMap_.count(fileDir) ? baseDirMap_[fileDir] : permission)) {
            return false;
        };
        dirStack.pop();
    }
    return true;
}

std::string FileUtils::GetParentDir(const std::string &fileDir)
{
    auto curDir = std::filesystem::path(fileDir);
    return curDir.parent_path().string();
}

std::string FileUtils::GetCurrentDir(const std::string &fileDir)
{
    // 兼容传递的文件夹路径，末尾带0-N个'/'均ok
    if (fileDir.length() > 0 && fileDir.back() == '/') {
        return GetParentDir(fileDir);
    }
    return fileDir;
}

// 按需创建基础目录，并且将需要创建的基础目录放在baseDirMap中，用于后续创建目录判断权限
void FileUtils::InitAndCreateBaseDirs(const std::vector<DirInfo> &dirInfos)
{
    if (dirInfos.empty()) {
        ENGINE_LOGE("dirInfo is empty");
        return;
    }
    for (const auto &dirInfo : dirInfos) {
        baseDirMap_[dirInfo.dirName] = dirInfo.dirPermissions;
        if (!IsFileExist(dirInfo.dirName)) {
            CreatDirWithPermission(dirInfo.dirName, dirInfo.dirPermissions);
        }
    }
}

void FileUtils::DestroyBaseDirectory(const std::vector<DirInfo> &dirInfos)
{
    ENGINE_LOGI("destroy base directory");
    if (dirInfos.empty()) {
        ENGINE_LOGE("dirInfo is empty");
        return;
    }

    for (const auto &dir : dirInfos) {
        if (dir.isAllowDestroyContents) {
            DeleteFile(dir.dirName, false);
        }
    }
}

bool FileUtils::CreatDirWithPermission(const std::string &fileDir, int32_t dirPermission)
{
    if (fileDir.empty() || strstr(fileDir.c_str(), "/.") != NULL || strstr(fileDir.c_str(), "./") != NULL) {
        ENGINE_LOGE("dirName %{public}s is invalid", fileDir.c_str());
        return false;
    }
    std::error_code errorCode;
    std::filesystem::create_directory(fileDir, errorCode);
    if (errorCode.operator bool()) {
        ENGINE_LOGE("Create directory dir[%s] fail, error message : %{public}s", fileDir.c_str(),
            errorCode.message().c_str());
        return false;
    }
    std::filesystem::permissions(fileDir, static_cast<std::filesystem::perms>(dirPermission),
        std::filesystem::perm_options::replace, errorCode);
    if (errorCode.operator bool()) {
        ENGINE_LOGE("Assign permissions failed, path = %{public}s,error message : %{public}s", fileDir.c_str(),
            errorCode.message().c_str());
        return false;
    }
    return true;
}

std::string FileUtils::GetFileOpenMode(OpenMode mode, StreamMode streamMode)
{
    static const std::map<OpenMode, std::string> modeMap = {
        { OpenMode::READ_ONLY, "r" },   { OpenMode::WRITE_CREATE, "w" },       { OpenMode::APPEND_CREATE, "a" },
        { OpenMode::READ_WRITE, "r+" }, { OpenMode::READ_WRITE_CREATE, "w+" }, { OpenMode::READ_APPEND_CREATE, "a+" },
    };
    std::string realMode;
    auto iter = modeMap.find(mode);
    if (iter == modeMap.end()) {
        ENGINE_LOGE("GetFileOpenMode failed, invalid mode: %{public}d", mode);
        return realMode;
    }
    realMode = iter->second;
    if (streamMode == StreamMode::BINARY) {
        realMode.insert(1, "b");
    }
    return realMode;
}

std::string FileUtils::ReadDataFromFile(const std::string &filePath)
{
    char dealPath[PATH_MAX] = {};
    if (realpath(filePath.c_str(), dealPath) == nullptr) {
        ENGINE_LOGE("filePath %{private}s is not exist or invalid", filePath.c_str());
        return "";
    }
    std::ifstream readFile;
    readFile.open(dealPath);
    if (readFile.fail()) {
        ENGINE_LOGI("open file from %{public}s err", filePath.c_str());
        return "";
    }
    std::stringstream streamBuffer;
    streamBuffer << readFile.rdbuf();
    std::string fileRaw(streamBuffer.str());
    readFile.close();
    return fileRaw;
}

std::string FileUtils::GetFileRealPath(const std::string &filePath)
{
    if (filePath.empty()) {
        ENGINE_LOGE("GetFileRealPath, file path is empty");
        return "";
    }

    size_t lastSlashPos = filePath.find_last_of('/');
    if (lastSlashPos == std::string::npos) {
        ENGINE_LOGE("GetFileRealPath, file path format is error");
        return "";
    }

    std::string parentDir = filePath.substr(0, lastSlashPos);
    char realPath[PATH_MAX] = {};
    if (realpath(parentDir.c_str(), realPath) == NULL) {
        ENGINE_LOGE("file path not exist or no permission to access the path");
        return "";
    }

    std::string fileName = filePath.substr(lastSlashPos + 1);
    return std::string(realPath) + "/" + fileName;
}

std::shared_ptr<FILE> FileUtils::CreateFileSharedPtr(const std::string &filePath, OpenMode openMode, int32_t permission,
    StreamMode streamMode)
{
    auto fileRealPath = GetFileRealPath(filePath);
    if (fileRealPath.empty()) {
        return nullptr;
    }

    std::string realMode = GetFileOpenMode(openMode, streamMode);
    FILE *fp = fopen(fileRealPath.c_str(), realMode.c_str());
    if (fp == nullptr) {
        ENGINE_LOGE("OpenFile failed, errno: %{public}d", errno);
        return nullptr;
    }
    std::shared_ptr<FILE> fileSharedPtr(fp, [filePath](FILE *ptr) {
        if (ptr != nullptr) {
            ENGINE_LOGI("close file, filePath: %{private}s", filePath.c_str());
            fclose(ptr);
            ptr = nullptr;
        }
    });

    if (permission != -1 && chmod(filePath.c_str(), permission) != 0) {
        ENGINE_LOGE("OpenFile failed, chmod error: %{public}d", errno);
        return nullptr;
    }
    return fileSharedPtr;
}
} // namespace UpdateService
} // namespace OHOS
