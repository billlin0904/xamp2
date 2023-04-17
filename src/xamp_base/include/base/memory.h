//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <string>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

#ifdef __cpp_lib_assume_aligned

#define AssumeAligned std::assume_aligned

#else

template <size_t AlignedBytes, typename T>
XAMP_ALWAYS_INLINE constexpr T* AssumeAligned(T* ptr) {
#ifdef XAMP_OS_MAC
	return reinterpret_cast<T*>(__builtin_assume_aligned(ptr, AlignedBytes));
#else
	if ((reinterpret_cast<std::uintptr_t>(ptr) & ((1 << AlignedBytes) - 1)) == 0) {
		return ptr;
	}
	__assume(false);
#endif
}

#endif


class MemoryMappedFile;

inline constexpr size_t kMaxPreReadFileSize = 65536;

XAMP_BASE_API size_t GetPageSize() noexcept;

XAMP_BASE_API size_t GetPageAlignSize(size_t value) noexcept;

XAMP_BASE_API bool PrefetchFile(std::wstring const& file_path);

XAMP_BASE_API bool PrefetchFile(MemoryMappedFile& file, size_t prefech_size = kMaxPreReadFileSize);

XAMP_BASE_API bool PrefetchMemory(void* adddr, size_t length) noexcept;

#define MemorySet(dest, c, size) std::memset(dest, c, size)
#define MemoryCopy(dest, src, size) std::memcpy(dest, src, size)

XAMP_BASE_NAMESPACE_END

