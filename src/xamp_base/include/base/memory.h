//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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

/*
* Allocate aligned memory.
*
* @param[in] size
* @param[in] aligned_size
* @return void*
*/
XAMP_BASE_API void* AlignedMalloc(size_t size, size_t aligned_size) noexcept;

/*
* Free aligned memory.
*
* @param[in] p
* @return void
* @note p must be allocated by AlignedMalloc.
*/
XAMP_BASE_API void AlignedFree(void* p) noexcept;

/*
* Allocate stack memory.
*
* @param[in] size
* @return void*
* @note StackAlloc is not thread safe.
*/
XAMP_BASE_API void* StackAlloc(size_t size);

/*
* Free stack memory.
*
* @param[in] p
* @return void
* @note p must be allocated by StackAlloc.
*/
XAMP_BASE_API void StackFree(void* p);

/*
* Align value to aligned_size.
*
* @param[in] value
* @param[in] aligned_size
* @return T
* @note aligned_size must be power of 2.
*/
template <typename T>
constexpr T AlignUp(T value, size_t aligned_size = kMallocAlignSize) {
    return T((value + (T(aligned_size) - 1)) & ~T(aligned_size - 1));
}

/*
* Allocate aligned memory.
*
* @param[in] aligned_size
* @return void*
* @note aligned_size must be power of 2.
*/
template <typename T>
XAMP_BASE_API_ONLY_EXPORT T* AlignedMallocOf(size_t aligned_size) noexcept {
    return static_cast<T*>(AlignedMalloc(sizeof(T), aligned_size));
}

/*
* Allocate aligned memory.
*
* @param[in] n
* @param[in] aligned_size
* @return void*
* @note aligned_size must be power of 2.
*/
template <typename Type>
XAMP_BASE_API_ONLY_EXPORT Type* AlignedMallocCountOf(size_t n, size_t aligned_size) noexcept {
    return static_cast<Type*>(AlignedMalloc(sizeof(Type) * n, aligned_size));
}

/*
* Aligned deleter.
*
* @param[in] p
* @return void
*/
template <typename Type>
struct XAMP_BASE_API_ONLY_EXPORT AlignedDeleter {
    void operator()(Type* p) const noexcept {
        AlignedFree(p);
    }
};

/*
* Aligned class deleter.
*
* @param[in] p
* @return void
*/
template <typename Type>
struct XAMP_BASE_API_ONLY_EXPORT AlignedClassDeleter {
    void operator()(Type* p) const {
        p->~Type();
        AlignedFree(p);
    }
};

/*
* Stack buffer deleter.
*
* @param[in] p
* @return void
*/
template <typename Type>
struct XAMP_BASE_API_ONLY_EXPORT StackBufferDeleter {
    void operator()(Type* p) const noexcept {
        StackFree(p);
    }
};

/*
* Free deleter.
*
* @param[in] p
* @return void
*/
template <typename Type>
struct XAMP_BASE_API_ONLY_EXPORT FreeDeleter {
    void operator()(Type* p) const noexcept {
        free(p);
    }
};

using CharPtr = std::unique_ptr<char, FreeDeleter<char>>;

template <typename Type>
using StackBufferPtr = std::unique_ptr<Type[], StackBufferDeleter<Type>>;

template <typename Type>
using AlignPtr = std::unique_ptr<Type, AlignedClassDeleter<Type>>;

template <typename Type>
using AlignArray = std::unique_ptr<Type[], AlignedDeleter<Type>>;

/*
* Make aligned pointer.
*
* @param[in] args
* @return AlignPtr<Type>
* @note Type must be default constructor.
*/
template <typename BaseType, typename ImplType, typename... Args, size_t AlignSize = kMallocAlignSize>
XAMP_BASE_API_ONLY_EXPORT AlignPtr<BaseType> MakeAlign(Args&& ... args) {
    auto* ptr = AlignedMallocOf<ImplType>(AlignSize);
    if (!ptr) {
        throw std::bad_alloc();
    }

    try {
        BaseType* base = ::new(ptr) ImplType(std::forward<Args>(args)...);
        return AlignPtr<BaseType>(base);
    }
    catch (...) {
        AlignedFree(ptr);
        throw;
    }
}

/*
* Make aligned pointer.
*
* @param[in] args
* @return AlignPtr<Type>
* @note Type must be default constructor.
*/
template <typename Type, typename... Args, size_t AlignSize = kMallocAlignSize>
XAMP_BASE_API_ONLY_EXPORT AlignPtr<Type> MakeAlign(Args&& ... args) {
    auto* ptr = AlignedMallocOf<Type>(AlignSize);
    if (!ptr) {
        throw std::bad_alloc();
    }

    try {
        return AlignPtr<Type>(::new(ptr) Type(std::forward<Args>(args)...));
    }
    catch (...) {
        AlignedFree(ptr);
        throw;
    }
}

/*
* Make aligned array.
*
* @param[in] n
* @return AlignArray<Type>
*/
template <typename Type>
XAMP_BASE_API_ONLY_EXPORT AlignArray<Type> MakeAlignedArray(size_t n) {
    auto ptr = AlignedMallocCountOf<Type>(n, kMallocAlignSize);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return AlignArray<Type>(static_cast<Type*>(ptr));
}

/*
* Make stack buffer.
*
* @param[in] n
* @return StackBufferPtr<Type>
* @note StackAlloc is not thread safe.
*/
template <typename Type = std::byte>
XAMP_BASE_API_ONLY_EXPORT StackBufferPtr<Type> MakeStackBuffer(size_t n) {
    auto* ptr = StackAlloc(sizeof(Type) * n);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return StackBufferPtr<Type>(static_cast<Type*>(ptr));
}

XAMP_BASE_NAMESPACE_END

