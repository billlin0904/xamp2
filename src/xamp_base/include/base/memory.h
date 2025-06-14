//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <string>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

class MemoryMappedFile;

inline constexpr size_t kMaxPreReadFileSize = 65536;

XAMP_BASE_API size_t GetPageSize() noexcept;

XAMP_BASE_API bool PrefetchFile(std::wstring const& file_path);

XAMP_BASE_API bool PrefetchFile(MemoryMappedFile& file, size_t prefech_size = kMaxPreReadFileSize);

XAMP_BASE_API bool PrefetchMemory(void* adddr, size_t length) noexcept;

#define MemorySet(dest, c, size) std::memset(dest, c, size)
#define MemoryCopy(dest, src, size) std::memcpy(dest, src, size)
#define MemoryMove(dest, src, size) std::memmove(dest, src, size)

XAMP_BASE_API XAMP_CHECK_LIFETIME void* AlignedMalloc(size_t size, size_t aligned_size) noexcept;

XAMP_BASE_API void AlignedFree(void* p) noexcept;

XAMP_BASE_API XAMP_CHECK_LIFETIME void* StackAlloc(size_t size);

XAMP_BASE_API void StackFree(void* p);

template <typename T>
constexpr T AlignUp(T value, size_t aligned_size = kMallocAlignSize) {
    return T((value + (T(aligned_size) - 1)) & ~T(aligned_size - 1));
}

template <typename T>
XAMP_BASE_API_ONLY_EXPORT XAMP_CHECK_LIFETIME T* AlignedMallocObject(size_t aligned_size) noexcept {
    return static_cast<T*>(AlignedMalloc(sizeof(T), aligned_size));
}

template <typename Type>
XAMP_BASE_API_ONLY_EXPORT XAMP_CHECK_LIFETIME Type* AlignedMallocArray(size_t n, size_t aligned_size) noexcept {
    return static_cast<Type*>(AlignedMalloc(sizeof(Type) * n, aligned_size));
}

template <typename Type>
struct XAMP_BASE_API_ONLY_EXPORT AlignedDeleter {
    void operator()(Type* p) const noexcept {
        AlignedFree(p);
    }
};

template <typename Type>
struct XAMP_BASE_API_ONLY_EXPORT AlignedClassDeleter {
    void operator()(Type* p) const {
        if constexpr (!std::is_trivially_destructible_v<Type>) {
            p->~Type();
        }        
        AlignedFree(p);
    }
};

template <typename Type>
struct XAMP_BASE_API_ONLY_EXPORT StackBufferDeleter {
    void operator()(Type* p) const noexcept {
        StackFree(p);
    }
};

template <typename Type>
struct XAMP_BASE_API_ONLY_EXPORT FreeDeleter {
    void operator()(Type* p) const noexcept {
        free(p);
    }
};

template <typename Type>
using StackBuffer = std::unique_ptr<Type[], StackBufferDeleter<Type>>;

template <typename Type>
using ScopedPtr = std::unique_ptr<Type, AlignedClassDeleter<Type>>;

using CharPtr = std::unique_ptr<char, FreeDeleter<char>>;

template <typename Type>
using ScopedArray = std::unique_ptr<Type[], AlignedDeleter<Type>>;

/*
* Make aligned pointer.
*
* @param[in] args
* @return ScopedPtr<Type>
* @note Type must be default constructor.
*/
template <typename BaseType, typename ImplType, typename... Args, size_t AlignSize = kMallocAlignSize>
XAMP_BASE_API_ONLY_EXPORT ScopedPtr<BaseType> MakeAlign(Args&& ... args) {
    auto ptr = ScopedPtr<void>(AlignedMallocObject<ImplType>(AlignSize));
    if (!ptr) {
        throw std::bad_alloc();
    }

    BaseType* obj = nullptr;
    try {
        obj = ::new(ptr.get()) ImplType(std::forward<Args>(args)...);
    }
    catch (...) {
        throw;
    }
	ptr.release();
    return ScopedPtr<BaseType>(obj);
}

/*
* Make aligned pointer.
*
* @param[in] args
* @return ScopedPtr<Type>
* @note Type must be default constructor.
*/
template <typename Type, typename... Args, size_t AlignSize = kMallocAlignSize>
XAMP_BASE_API_ONLY_EXPORT ScopedPtr<Type> MakeAlign(Args&& ... args) {
    auto ptr = ScopedPtr<void>(AlignedMallocObject<Type>(AlignSize));
    if (!ptr) {
        throw std::bad_alloc();
    }

    Type* obj = nullptr;
    try {
        obj = ::new(ptr.get()) Type(std::forward<Args>(args)...);
    }
    catch (...) {        
        throw;
    }
    ptr.release();
    return ScopedPtr<Type>(obj);
}

template <typename BaseType, typename ImplType, typename... Args>
std::shared_ptr<BaseType> MakeShared(Args&&... args) {
    constexpr size_t AlignSize = alignof(ImplType);
    void* mem = ::operator new(sizeof(ImplType), std::align_val_t{ AlignSize });
    if (!mem) {
        throw std::bad_alloc();
    }

    try {
        auto obj = ::new(mem) ImplType(std::forward<Args>(args)...);
        return std::shared_ptr<BaseType>(obj, [](BaseType* ptr) {
            ptr->~BaseType();
            ::operator delete(ptr, std::align_val_t{ AlignSize });
            });
    }
    catch (...) {
        ::operator delete(mem, std::align_val_t{ AlignSize });
        throw;
    }
}

/*
* Make aligned array.
*
* @param[in] n
* @return ScopedArray<Type>
*/
template <typename Type>
XAMP_BASE_API_ONLY_EXPORT ScopedArray<Type> MakeAlignedArray(size_t n) {
    auto ptr = AlignedMallocArray<Type>(n, kMallocAlignSize);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return ScopedArray<Type>(static_cast<Type*>(ptr));
}

/*
* Make stack buffer.
*
* @param[in] n
* @return StackBuffer<Type>
* @note StackAlloc is not thread safe.
*/
template <typename Type = std::byte>
XAMP_BASE_API_ONLY_EXPORT StackBuffer<Type> MakeStackBuffer(size_t n) {
    auto* ptr = StackAlloc(sizeof(Type) * n);
    if (!ptr) {
            throw std::bad_alloc();
    }
    return StackBuffer<Type>(static_cast<Type*>(ptr));
}

XAMP_BASE_NAMESPACE_END

