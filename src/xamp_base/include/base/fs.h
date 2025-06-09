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

using CFilePtr = std::unique_ptr<FILE, decltype(&fclose)>;

class XAMP_BASE_API CTemporaryFile {
public:
    CTemporaryFile();

    ~CTemporaryFile();

    XAMP_ALWAYS_INLINE size_t Read(void* buffer, size_t size, size_t count) noexcept {
        return std::fread(buffer, size, count, file());
    }

    XAMP_ALWAYS_INLINE size_t Write(const void* buffer, size_t size, size_t count) noexcept {
        return std::fwrite(buffer, size, count, file());
    }

    bool Seek(uint64_t off, int32_t origin) noexcept;

    uint64_t Tell() noexcept;

    void Close() noexcept;
    
private:    
    FILE* file() noexcept {
        return file_.get();
    }
    
    std::tuple<CFilePtr, Path> GetTempFile();

    Path path_;
    CFilePtr file_;
};

class XAMP_BASE_API TemporaryFile {
public:
    TemporaryFile();

    XAMP_PIMPL(TemporaryFile)

    size_t Read(void* buffer, size_t size, size_t count) noexcept;

    size_t Write(const void* buffer, size_t size, size_t count) noexcept;

    bool Seek(uint64_t off, int32_t origin) noexcept;

    uint64_t Tell() noexcept;

    void Close() noexcept;

private:
    class TemporaryFileImpl;
    ScopedPtr<TemporaryFileImpl> impl_;
};

class XAMP_BASE_API FastIOStream {
public:
    enum class Mode { Read, ReadWrite };

    FastIOStream(const Path& file_path, Mode m = Mode::Read);

    XAMP_PIMPL(FastIOStream)

    std::size_t read(void* dst, std::size_t len);

    std::size_t write(const void* src, std::size_t len);

    void seek(int64_t off, int whence);

    uint64_t tell() const;

    uint64_t size() const;

    void truncate(uint64_t new_size);

    [[nodiscard]] bool is_open() const;

    [[nodiscard]] bool read_only() const;

    const Path& path() const;
private:
    class FastIOStreamImpl;
    ScopedPtr<FastIOStreamImpl> impl_;
};

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
