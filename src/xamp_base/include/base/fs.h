//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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

XAMP_BASE_API bool IsFilePath(const std::wstring& file_path) noexcept;

XAMP_BASE_API std::string GetSharedLibraryName(const std::string_view &name);

XAMP_BASE_API std::string MakeTempFileName();

XAMP_BASE_API Path GetTempFileNamePath();

XAMP_BASE_API Path GetApplicationFilePath();

XAMP_BASE_API Path GetComponentsFilePath();

XAMP_BASE_API int64_t GetLastWriteTime(const Path &path);

XAMP_BASE_API bool IsCDAFile(const Path& path);

/*
* Imbue file from bom.
* 
* @param[in] file
*/
XAMP_BASE_API void ImbueFileFromBom(std::wifstream& file);

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
        return TryImpl(std::forward<Func>(func), [](std::exception const&) {});
    }

    /*
    * Try to write file.
    *
    * @param[in] func
    * @param[in] exception_handler
    * @return bool
    */
    template <typename Func, typename ExceptionHandler>
    bool Try(Func&& func, ExceptionHandler&& exception_handler) {
        return TryImpl(std::forward<Func>(func), std::forward<ExceptionHandler>(exception_handler));
    }

private:
    /*
    * Try to write file.
    * 
    * @param[in] func
    * @param[in] exception_handler
    * @return bool
    */
    template <typename Func, typename ExceptionHandler>
    bool TryImpl(Func&& func, ExceptionHandler&& exception_handler) {
        // Create temp file path.
        temp_file_path_ = Fs::temp_directory_path()
            / Fs::path(MakeTempFileName() + ".tmp");

        try {
            // Write file.
            func(temp_file_path_);
            // Rename file.
            Fs::rename(temp_file_path_, dest_file_path_);
            return true;
        }
        catch (Exception const& e) {
            exception_handler(e);
        }
        catch (std::exception const& e) {
            exception_handler(e);
        }
        catch (...) {
            exception_handler(std::runtime_error("Unknown exception"));
        }
        // Remove temp file.
        Fs::remove(temp_file_path_);
        return false;
    }
    Path dest_file_path_;
    Path temp_file_path_;
};

XAMP_BASE_NAMESPACE_END
