//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/exception.h>

#include <base/text_encoding.h>
#include <expected>
#include <filesystem>

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

XAMP_BASE_API inline [[nodiscard]] bool IsFileReadOnly(const Path& path) {
    return (Fs::status(path).permissions() & Fs::perms::owner_read) != Fs::perms::none;
}

XAMP_BASE_API bool IsFilePath(const Path& file_path) noexcept;

XAMP_BASE_API std::string GetSharedLibraryName(const std::string_view &name);

XAMP_BASE_API Path GetTempFileNamePath();

XAMP_BASE_API std::tuple<std::fstream, Path> GetTempFile();

XAMP_BASE_API Path GetApplicationFilePath();

XAMP_BASE_API Path GetComponentsFilePath();

XAMP_BASE_API bool IsCDAFile(const Path& path);

XAMP_BASE_API bool IsFileOnSsd(const Path& path);

XAMP_BASE_API std::expected<std::string, TextEncodeingError> ReadFileToUtf8String(const Path& path);

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
        return TryImpl(std::forward<Func>(func));
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
    bool TryImpl(Func&& func) {
        // Create temp file path.
        temp_file_path_ = GetTempFileNamePath();

        try {
            // Write file.
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
