//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <Windows.h>

#include <base/unique_handle.h>

namespace xamp::base {

struct XAMP_BASE_API HandleTraits final {
	static HANDLE invalid() noexcept {
		return nullptr;
	}

	static void close(HANDLE value) noexcept {
		::CloseHandle(value);
	}
};

struct XAMP_BASE_API FileHandleTraits final {
	static HANDLE invalid() noexcept {
		return INVALID_HANDLE_VALUE;
	}

	static void close(HANDLE value) noexcept {
		::CloseHandle(value);
	}
};

struct XAMP_BASE_API ModuleHandleTraits final {
    static HMODULE invalid() noexcept {
        return nullptr;
    }

    static void close(HMODULE value) noexcept {
        ::FreeLibrary(value);
    }
};

struct XAMP_BASE_API MappingFileHandleTraits final {
	static HANDLE invalid() noexcept {
		return nullptr;
	}

	static void close(HANDLE value) noexcept {
		::CloseHandle(value);
	}
};

struct XAMP_BASE_API MappingMemoryAddressTraits final {
	static void* invalid() noexcept {
		return nullptr;
	}

	static void close(void* value) noexcept {
		::UnmapViewOfFile(value);
	}
};

struct XAMP_BASE_API WorkDeleter final {
	static PTP_WORK invalid() noexcept { return nullptr; }
	static void close(PTP_WORK value) { ::CloseThreadpoolWork(value); }
};

struct XAMP_BASE_API ThreadPoolDeleter final {
	static PTP_POOL invalid() noexcept { return nullptr; }
	static void close(PTP_POOL value) { ::CloseThreadpool(value); }
};

struct XAMP_BASE_API CleanupThreadGroupDeleter final {
	static PTP_CLEANUP_GROUP invalid() noexcept { return nullptr; }
	static void close(PTP_CLEANUP_GROUP value) { ::CloseThreadpoolCleanupGroup(value); }
};

using WorkHandle = UniqueHandle<PTP_WORK, WorkDeleter>;
using CleanupThreadGroupHandle = UniqueHandle<PTP_CLEANUP_GROUP, CleanupThreadGroupDeleter>;
using ThreadPoolHandle = UniqueHandle<PTP_POOL, ThreadPoolDeleter>;

using WinHandle = UniqueHandle<HANDLE, HandleTraits>;
using ModuleHandle = UniqueHandle<HMODULE, ModuleHandleTraits>;
using FileHandle = UniqueHandle<HANDLE, FileHandleTraits>;
using MappingFileHandle = UniqueHandle<HANDLE, MappingFileHandleTraits>;
using MappingAddressHandle = UniqueHandle<void*, MappingMemoryAddressTraits>;

}
#endif
