//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/fs.h>

XAMP_BASE_NAMESPACE_BEGIN

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
    enum class Mode { Read, ReadWrite, ReadWriteOnlyExisting };

    FastIOStream();

    FastIOStream(const Path& file_path, Mode m = Mode::Read);

    void open(const Path& file_path, Mode m = Mode::Read);

    XAMP_PIMPL(FastIOStream)

    size_t read(void* dst, size_t len);

    size_t write(const void* src, size_t len);

    void seek(int64_t off, int whence);

    uint64_t tell() const;

    uint64_t size() const;

    void truncate(uint64_t new_size);

    [[nodiscard]] bool is_open() const;

    [[nodiscard]] bool read_only() const;

    const Path& path() const;

    void close();

private:
    class FastIOStreamImpl;
    ScopedPtr<FastIOStreamImpl> impl_;
};

XAMP_BASE_NAMESPACE_END

