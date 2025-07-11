//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#ifndef STRICT
#	define STRICT 1
#endif

#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#	define NOMINMAX
#endif

#include <Windows.h>

#include <base/unique_handle.h>

XAMP_BASE_NAMESPACE_BEGIN

struct XAMP_BASE_API HandleTraits final {
	static HANDLE invalid() noexcept;

	static void Close(HANDLE value) noexcept;
};

struct XAMP_BASE_API FileHandleTraits final {
	static HANDLE invalid() noexcept;

	static void Close(HANDLE value) noexcept;
};

struct XAMP_BASE_API ModuleHandleTraits final {
	static HMODULE invalid() noexcept;

	static void Close(HMODULE value) noexcept;
};

struct XAMP_BASE_API MappingFileHandleTraits final {
	static HANDLE invalid() noexcept;

	static void Close(HANDLE value) noexcept;
};

struct XAMP_BASE_API MappingMemoryAddressTraits final {
	static void* invalid() noexcept;

	static void Close(void* value) noexcept;
};

struct XAMP_BASE_API TimerQueueTraits final {
	static HANDLE invalid() noexcept;

	static void Close(HANDLE value) noexcept;
};

struct XAMP_BASE_API RegTraits final {
	static HKEY invalid() noexcept;

	static void Close(HKEY value) noexcept;
};

using WinHandle = UniqueHandle<HANDLE, HandleTraits>;
using SharedLibraryHandle = UniqueHandle<HINSTANCE, ModuleHandleTraits>;
using FileHandle = UniqueHandle<HANDLE, FileHandleTraits>;
using MappingFileHandle = UniqueHandle<HANDLE, MappingFileHandleTraits>;
using MappingAddressHandle = UniqueHandle<void*, MappingMemoryAddressTraits>;
using TimerQueueHandle = UniqueHandle<HANDLE, TimerQueueTraits>;
using RegHandle = UniqueHandle<HKEY, RegTraits>;

XAMP_BASE_NAMESPACE_END

#endif
