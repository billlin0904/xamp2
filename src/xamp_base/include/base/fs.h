//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/exception.h>

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

XAMP_BASE_API Path GetApplicationFilePath();

XAMP_BASE_API Path GetComponentsFilePath();

XAMP_BASE_API int64_t GetLastWriteTime(const Path &path);

XAMP_BASE_API bool IsCDAFile(Path const& path);

class XAMP_BASE_API ExceptedFile final {
public:
    explicit ExceptedFile(Path const& dest_file_path) {
        dest_file_path_ = dest_file_path;
        temp_file_path_ = Fs::temp_directory_path()
            / Fs::path(MakeTempFileName() + ".tmp");
    }

    template <typename Func>
    bool Try(Func&& func) {
        return Try(func, DefaultExceptionHandler);
    }

    template <typename Func, typename ExceptionHandler>
    bool Try(Func&& func, ExceptionHandler && exception_handler) {
        try {
            func(temp_file_path_);
            Fs::rename(temp_file_path_, dest_file_path_);
            return true;
        }
        catch (Exception const& e) {
            exception_handler(e);
        }
        catch (...) {
        }
        Fs::remove(temp_file_path_);
        return false;
    }

private:
    static void DefaultExceptionHandler(Exception const&) {	    
    }
    Path dest_file_path_;
    Path temp_file_path_;
};

}
