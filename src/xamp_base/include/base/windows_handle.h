//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <base/unique_handle.h>

namespace xamp::base {

struct XAMP_BASE_API HandleTraits final {
	static HANDLE invalid() noexcept;

	static void close(HANDLE value) noexcept;
};

struct XAMP_BASE_API FileHandleTraits final {
	static HANDLE invalid() noexcept;

	static void close(HANDLE value) noexcept;
};

struct XAMP_BASE_API ModuleHandleTraits final {
	static HMODULE invalid() noexcept;

	static void close(HMODULE value) noexcept;
};

struct XAMP_BASE_API MappingFileHandleTraits final {
	static HANDLE invalid() noexcept;

	static void close(HANDLE value) noexcept;
};

struct XAMP_BASE_API MappingMemoryAddressTraits final {
	static void* invalid() noexcept;

	static void close(void* value) noexcept;
};

struct XAMP_BASE_API WorkTraits final {
	static PTP_WORK invalid() noexcept;

	static void close(PTP_WORK value);
};

struct XAMP_BASE_API ThreadPoolTraits final {
	static PTP_POOL invalid() noexcept;

	static void close(PTP_POOL value);
};

struct XAMP_BASE_API CleanupThreadGroupTraits final {
	static PTP_CLEANUP_GROUP invalid() noexcept;

	static void close(PTP_CLEANUP_GROUP value);
};

struct XAMP_BASE_API TimerQueueTraits final {
	static HANDLE invalid() noexcept;

	static void close(HANDLE value);
};

using WorkHandle = UniqueHandle<PTP_WORK, WorkTraits>;
using CleanupThreadGroupHandle = UniqueHandle<PTP_CLEANUP_GROUP, CleanupThreadGroupTraits>;
using ThreadPoolHandle = UniqueHandle<PTP_POOL, ThreadPoolTraits>;

using WinHandle = UniqueHandle<HANDLE, HandleTraits>;
using ModuleHandle = UniqueHandle<HMODULE, ModuleHandleTraits>;
using FileHandle = UniqueHandle<HANDLE, FileHandleTraits>;
using MappingFileHandle = UniqueHandle<HANDLE, MappingFileHandleTraits>;
using MappingAddressHandle = UniqueHandle<void*, MappingMemoryAddressTraits>;
using TimerQueueHandle = UniqueHandle<HANDLE, TimerQueueTraits>;

}
#endif
