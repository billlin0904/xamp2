//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef _WIN32
#include <Windows.h>

#include <base/unique_handle.h>

namespace xamp::base {

struct HandleTraits final {
	static HANDLE invalid() noexcept {
		return nullptr;
	}

	static void close(HANDLE value) noexcept {
		CloseHandle(value);
	}
};

struct FileHandleTraits final {
	static HANDLE invalid() noexcept {
		return INVALID_HANDLE_VALUE;
	}

	static void close(HANDLE value) noexcept {
		CloseHandle(value);
	}
};

struct ModuleHandleTraits final {
    static HMODULE invalid() noexcept {
        return nullptr;
    }

    static void close(HMODULE value) noexcept {
        FreeLibrary(value);
    }
};

struct MappingFileHandleTraits final {
	static HANDLE invalid() noexcept {
		return nullptr;
	}

	static void close(HANDLE value) noexcept {
		CloseHandle(value);
	}
};

struct MappingMemoryAddressTraits final {
	static void* invalid() noexcept {
		return nullptr;
	}

	static void close(void* value) noexcept {
		UnmapViewOfFile(value);
	}
};

using WinHandle = UniqueHandle<HANDLE, HandleTraits>;
using ModuleHandle = UniqueHandle<HMODULE, ModuleHandleTraits>;
using FileHandle = UniqueHandle<HANDLE, FileHandleTraits>;
using MappingFileHandle = UniqueHandle<HANDLE, MappingFileHandleTraits>;
using MappingAddressHandle = UniqueHandle<void*, MappingMemoryAddressTraits>;

}
#endif
