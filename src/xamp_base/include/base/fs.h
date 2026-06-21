//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/exception.h>

#include <base/text_encoding.h>

#include <expected>
#include <filesystem>
#include <span>

XAMP_BASE_NAMESPACE_BEGIN

namespace Fs = std::filesystem;
using RecursiveDirectoryIterator = Fs::recursive_directory_iterator;
using DirectoryIterator = Fs::directory_iterator;
using DirectoryEntry = Fs::directory_entry;
using Path = Fs::path;

inline constexpr auto kIteratorOptions{
	std::filesystem::directory_options::follow_directory_symlink |
	std::filesystem::directory_options::skip_permission_denied
};

[[nodiscard]] XAMP_BASE_API inline bool isFileReadOnly(const Path& path) {
    std::error_code ec;
    const auto permissions = Fs::status(path, ec).permissions();
    if (ec) {
        return false;
    }
    return (permissions & Fs::perms::owner_write) == Fs::perms::none;
}

XAMP_BASE_API bool isFilePath(const Path& file_path) ;

XAMP_BASE_API std::string getSharedLibraryName(const std::string_view &name);

XAMP_BASE_API Path getTempFileNamePath();

XAMP_BASE_API std::tuple<std::fstream, Path> getTempFile();

XAMP_BASE_API Path getApplicationFilePath();

XAMP_BASE_API Path getComponentsFilePath();

XAMP_BASE_API bool IsCDAFile(const Path& path);

XAMP_BASE_API std::expected<std::string, TextEncodeingError> readFileToUtf8String(const Path& path);

XAMP_BASE_API std::expected<std::wstring, Errors> normalizePathToWideString(const Path& path);

/*
* Exception safe file.
* 
*/
class XAMP_BASE_API ExceptedFile final {
public:
    /*
    * Constructor.
    */
    explicit ExceptedFile(const Path& dest_file_path) {
        dest_file_path_ = dest_file_path;        
    }

    /*
    * Try to write file.
    * 
    * @param[in] func
    * @return bool  
    */
    template <typename Func>
    bool Try(Func&& func) {
        return tryImpl(std::forward<Func>(func));
    }

private:
    /*
    * Try to write file.
    * 
    * @param[in] func
    * @param[in] exception_handler
    * @return bool
    */
    template <typename Func>
    bool tryImpl(Func&& func) {
        // create temp file path.
        temp_file_path_ = getTempFileNamePath();

        try {
            // write file.
            func(temp_file_path_);
            // Rename file.
            Fs::rename(temp_file_path_, dest_file_path_);
            return true;
        }
        catch (...) {
            // Remove temp file.
            Fs::remove(temp_file_path_);
            std::rethrow_exception(std::current_exception());
        }
    }
    Path dest_file_path_;
    Path temp_file_path_;
};

XAMP_BASE_NAMESPACE_END
