//====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <thread>
#include <vector>

#include <base/enum.h>
#include <base/base.h>
#include <base/fs.h>
#include <base/memory.h>

XAMP_BASE_NAMESPACE_BEGIN
	XAMP_MAKE_ENUM(
    ThreadPriority,
    PRIORITY_UNKNOWN,
    PRIORITY_BACKGROUND,
    PRIORITY_NORMAL,
    PRIORITY_HIGHEST)

XAMP_MAKE_ENUM(
    ProcessPriority,
    PRIORITY_UNKNOWN,
    PRIORITY_BACKGROUND,
    PRIORITY_BACKGROUND_PERCEIVABLE,
    PRIORITY_FOREGROUND_KEYBOARD,
    PRIORITY_PREALLOC,
    PRIORITY_FOREGROUND,
    PRIORITY_FOREGROUND_HIGH,
    PRIORITY_PARENT_PROCESS
)

inline constexpr uint32_t kInfinity =
    #ifdef XAMP_OS_WIN 
		0xFFFFFFFF;
    #else
        0; // 在 macOS 上，超時為 0 表示無限等待
    #endif

enum FastFileOpenMode {
    FAST_IO_CREATE_ALWAYS = 0,
    FAST_IO_OPEN_EXISTING = 1,
    FAST_IO_READ = 2,
    FAST_IO_WRITE = 4,
    FAST_IO_READ_WRITE = (FAST_IO_READ | FAST_IO_WRITE),
};

enum FastFilSeekMode {
	FAST_IO_SEEK_SET = 0,
	FAST_IO_SEEK_CUR = 1,
	FAST_IO_SEEK_END = 2,
};

class XAMP_BASE_API XAMP_NO_VTABLE IPlatformFile {
public:
    XAMP_BASE_CLASS(IPlatformFile)

	virtual void Open(const Path& file_path, FastFileOpenMode mode) = 0;

    virtual int64_t Seek(int64_t offset, FastFilSeekMode mode) = 0;

    virtual void Close() = 0;

    virtual bool Read(void* buffer,
        uint32_t bytes_to_read, 
        uint32_t& bytes_read) = 0;

    virtual bool Write(const void* buffer,
        uint32_t bytes_to_write, 
        uint32_t& bytes_written) = 0;
protected:
    IPlatformFile() = default;
};

class XAMP_BASE_API FastFile : public IPlatformFile {
public:
    FastFile();

	explicit FastFile(const Path& file_path, FastFileOpenMode mode);

    virtual ~FastFile();

	void Open(const Path & file_path, FastFileOpenMode mode) override;

    int64_t Seek(int64_t offset, FastFilSeekMode mode) override;

	void Close() override;

    bool Read(void* buffer,
        uint32_t bytes_to_read,
        uint32_t& bytes_read) override;

    bool Write(const void* buffer,
        uint32_t bytes_to_write, 
        uint32_t& bytes_written) override;
private:
    class FastFileImpl;
	ScopedPtr<FastFileImpl> impl_;
};

XAMP_BASE_API void SetThreadPriority(std::jthread& thread,
    ThreadPriority priority);

XAMP_BASE_API void SetThreadName(std::wstring const & name);

XAMP_BASE_API std::string GetCurrentThreadId();

XAMP_BASE_API bool IsDebuging();

XAMP_BASE_API bool VirtualMemoryLock(void* address, size_t size);

XAMP_BASE_API bool VirtualMemoryUnLock(void* address, size_t size);

XAMP_BASE_API void MSleep(std::chrono::milliseconds timeout);

XAMP_BASE_API int32_t AtomicWait(std::atomic<uint32_t>& to_wait_on, 
    uint32_t expected, 
    const timespec* to) noexcept;

XAMP_BASE_API bool AtomicWait(std::atomic<uint32_t>& to_wait_on,
    uint32_t expected, 
    uint32_t milliseconds) noexcept;

XAMP_BASE_API void AtomicWakeSingle(std::atomic<uint32_t>& to_wake) noexcept;

XAMP_BASE_API void AtomicWakeAll(std::atomic<uint32_t>& to_wake) noexcept;

XAMP_BASE_API uint64_t GenRandomSeed() noexcept;

XAMP_BASE_API uint64_t GetSystemEntropy() noexcept;

XAMP_BASE_API void CpuRelax() noexcept;

XAMP_BASE_API void Assert(const char* message, const char* file, uint32_t line);

XAMP_BASE_API std::string GetSequentialUUID();

#ifdef XAMP_OS_WIN
XAMP_BASE_API void SetCurrentProcessPriority(ProcessPriority priority);

XAMP_BASE_API void SetProcessPriority(int32_t pid, ProcessPriority priority);

XAMP_BASE_API bool EnablePrivilege(std::string_view privilege, bool enable);

XAMP_BASE_API bool ExtendProcessWorkingSetSize(size_t size);

XAMP_BASE_API bool SetProcessWorkingSetSize(size_t working_set_size);

XAMP_BASE_API void SetProcessMitigation();

XAMP_BASE_API void SetThreadMitigation();

XAMP_BASE_API bool KillProcessByPidAndChildren(uint64_t pid);

XAMP_BASE_API bool KillProcessByNameAndChildren(const std::string& process_name);
#endif

XAMP_BASE_NAMESPACE_END
