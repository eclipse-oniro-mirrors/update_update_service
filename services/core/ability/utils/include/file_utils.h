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

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace OHOS {
namespace UpdateService {
constexpr int32_t FOLDER_PERMISSION = 0750;

struct DirInfo {
    std::string dirName;
    int32_t dirPermissions;
    bool isAllowDestroyContents = false; // 是否允许删除当前目录下的所有文件（包括子目录）
};

enum class OpenMode {
    READ_ONLY = 0,          // 只读, 文件必须存在
    WRITE_CREATE = 1,       // 只写, 有文件则清空,无文件则创建
    APPEND_CREATE = 2,      // 追加写, 有文件则追加,无文件则创建
    READ_WRITE = 3,         // 可读可写, 文件必须存在
    READ_WRITE_CREATE = 4,  // 可读可写, 有文件则清空,无文件则创建
    READ_APPEND_CREATE = 5, // 可读可写, 有文件则追加,无文件则创建
};

enum class StreamMode {
    TEXT = 0,   // 文本
    BINARY = 1, // 二进制
};

class FileUtils {
public:
    static bool IsFileExist(const std::string &fileName);
    static int64_t GetFileSize(const std::string &fileName);
    static int64_t RetrieveFileSize(const std::string &fileName);
    static bool IsSpaceEnough(const std::string &filePath, const int64_t requiredSpace);
    static bool SaveDataToFile(const std::string &filePath, const std::string &data);
    static void DeleteFile(const std::string &rootPath, bool isDeleteRootDir);
    static bool CreateMultiDirWithPermission(const std::string &fileDir, int32_t permission);
    static void InitAndCreateBaseDirs(const std::vector<DirInfo> &dirInfos);
    static void DestroyBaseDirectory(const std::vector<DirInfo> &dirInfos);
    static std::string ReadDataFromFile(const std::string &filePath);
    static std::string GetParentDir(const std::string &fileDir);
    static std::string GetFileRealPath(const std::string &filePath);
    static std::shared_ptr<FILE> CreateFileSharedPtr(const std::string &filePath,
        OpenMode openMode = OpenMode::READ_ONLY, int32_t permission = -1, StreamMode streamMode = StreamMode::BINARY);

private:
    static std::string GetCurrentDir(const std::string &fileDir);
    static bool CreatDirWithPermission(const std::string &fileDir, int32_t dirPermission);
    static std::string GetFileOpenMode(OpenMode mode, StreamMode streamMode);
    static void RemoveAll(const std::filesystem::path &path);

private:
    static std::map<std::string, int32_t> baseDirMap_;
};
} // namespace UpdateService
} // namespace OHOS
#endif // FILE_UTILS_H