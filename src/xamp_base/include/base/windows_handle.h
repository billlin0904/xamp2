//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
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
	static HANDLE invalid() ;

	static void Close(HANDLE value) ;
};

struct XAMP_BASE_API FileHandleTraits final {
	static HANDLE invalid() ;

	static void Close(HANDLE value) ;
};

struct XAMP_BASE_API ModuleHandleTraits final {
	static HMODULE invalid() ;

	static void Close(HMODULE value) ;
};

struct XAMP_BASE_API MappingFileHandleTraits final {
	static HANDLE invalid() ;

	static void Close(HANDLE value) ;
};

struct XAMP_BASE_API MappingMemoryAddressTraits final {
	static void* invalid() ;

	static void Close(void* value) ;
};

struct XAMP_BASE_API TimerQueueTraits final {
	static HANDLE invalid() ;

	static void Close(HANDLE value) ;
};

struct XAMP_BASE_API RegTraits final {
	static HKEY invalid() ;

	static void Close(HKEY value) ;
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
