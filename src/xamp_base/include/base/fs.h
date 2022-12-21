//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <filesystem>

#ifdef XAMP_OS_MAC
namespace std {
template <>
struct hash<filesystem::path> {
    size_t operator()(const filesystem::path& p) const noexcept {
        return filesystem::hash_value(p);
    }
};
}
#endif

namespace xamp::base {

namespace Fs = std::filesystem;
using RecursiveDirectoryIterator = Fs::recursive_directory_iterator;
using DirectoryIterator = Fs::directory_iterator;
using DirectoryEntry = Fs::directory_entry;
using Path = Fs::path;

inline constexpr auto kIteratorOptions{
	std::filesystem::directory_options::follow_directory_symlink |
	std::filesystem::directory_options::skip_permission_denied
};

XAMP_BASE_API bool IsFilePath(std::wstring const& file_path) noexcept;

XAMP_BASE_API std::string GetSharedLibraryName(const std::string_view &name);

XAMP_BASE_API std::string MakeTempFileName();

XAMP_BASE_API Path GetTempFilePath();

XAMP_BASE_API int64_t GetLastWriteTime(const Path &path);

}
