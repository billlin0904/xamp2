//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

class MemoryMappedFile;

inline constexpr size_t kMaxPreReadFileSize = 65536;

XAMP_BASE_API size_t getPageSize() ;

XAMP_BASE_API bool prefetchFile(std::wstring const& file_path);

XAMP_BASE_API bool prefetchFile(MemoryMappedFile& file_, size_t prefech_size = kMaxPreReadFileSize);

XAMP_BASE_API bool prefetchMemory(void* adddr, size_t length) ;

#define MemorySet(dest, c, size) std::memset(dest, c, size)
#define MemoryCopy(dest, src, size) std::memcpy(dest, src, size)
#define MemoryMove(dest, src, size) std::memmove(dest, src, size)

XAMP_BASE_API XAMP_CHECK_LIFETIME void* alignedMalloc(size_t size, size_t aligned_size) ;

XAMP_BASE_API void alignedFree(void* p);

// Stack allocation macro for platform-specific stack allocation.
// Use marco to avoid leave stack memory allocated by alloca or _malloca on the stack,
// which will be automatically freed when the function returns.
#define stackAlloc(size)  reinterpret_cast<std::byte*>(alloca(size))

XAMP_BASE_API void stackFree(void* p);

template <typename t>
constexpr t alignUp(t value, size_t aligned_size = kMallocAlignSize) {
    return t((value + (t(aligned_size) - 1)) & ~t(aligned_size - 1));
}

template <typename t>
XAMP_CHECK_LIFETIME t* alignedMallocObject(size_t aligned_size) {
    return static_cast<t*>(alignedMalloc(sizeof(t), aligned_size));
}

template <typename Type>
XAMP_CHECK_LIFETIME Type* alignedMallocArray(size_t n, size_t aligned_size) {
    return static_cast<Type*>(alignedMalloc(sizeof(Type) * n, aligned_size));
}

template <typename Type>
struct AlignedDeleter {
    void operator()(Type* p) const {
        alignedFree(p);
    }
};

template <typename Type>
struct AlignedClassDeleter {
    void operator()(Type* p) const {
        if constexpr (!std::is_trivially_destructible_v<Type>) {
            p->~Type();
        }        
        alignedFree(p);
    }
};

template <typename Type>
struct StackBufferDeleter {
    void operator()(Type* p) const {
        stackFree(p);
    }
};

template <typename Type>
struct FreeDeleter {
    void operator()(Type* p) const {
        ::free(p);
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
* @return ScopedPtr<BaseType>
*/
template <typename BaseType, typename ImplType, size_t AlignSize = kMallocAlignSize, typename... Args>
XAMP_CHECK_LIFETIME ScopedPtr<BaseType> makeAlign(Args&& ... args) {
    static_assert(std::is_base_of_v<BaseType, ImplType>, "ImplType must derive from BaseType.");
    static_assert(std::is_same_v<BaseType, ImplType> || std::has_virtual_destructor_v<BaseType>,
        "BaseType must have a virtual destructor when MakeAlign returns ScopedPtr<BaseType> for a derived ImplType.");
    static_assert((AlignSize & (AlignSize - 1)) == 0, "AlignSize must be a power of two.");

    constexpr auto kActualAlignSize = (std::max)(AlignSize, alignof(ImplType));
    auto* storage = alignedMallocObject<ImplType>(kActualAlignSize);
    if (!storage) {
        throw std::bad_alloc();
    }

    std::unique_ptr<void, AlignedDeleter<void>> guard{ storage };
    auto* obj = std::construct_at(storage, std::forward<Args>(args)...);
    guard.release();
    return ScopedPtr<BaseType>(static_cast<BaseType*>(obj));
}

/*
* Make aligned pointer.
*
* @param[in] args
* @return ScopedPtr<Type>
*/
template <typename Type, size_t AlignSize = kMallocAlignSize, typename... Args>
XAMP_CHECK_LIFETIME ScopedPtr<Type> makeAlign(Args&& ... args) {
    static_assert((AlignSize & (AlignSize - 1)) == 0, "AlignSize must be a power of two.");

    constexpr auto kActualAlignSize = (std::max)(AlignSize, alignof(Type));
    auto* storage = alignedMallocObject<Type>(kActualAlignSize);
    if (!storage) {
        throw std::bad_alloc();
    }

    std::unique_ptr<void, AlignedDeleter<void>> guard{ storage };
    auto* obj = std::construct_at(storage, std::forward<Args>(args)...);
    guard.release();
    return ScopedPtr<Type>(obj);
}

template <typename BaseType, typename ImplType, typename... Args>
std::shared_ptr<BaseType> makeShared(Args&&... args) {
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

template <typename Type>
ScopedArray<Type> makeAlignedArray(size_t n) {
    static_assert(std::is_trivially_copyable_v<Type>, "makeAlignedArray only supports trivially copyable types.");

    constexpr auto kActualAlignSize = (std::max)(kMallocAlignSize, alignof(Type));
    auto ptr = alignedMallocArray<Type>(n, kActualAlignSize);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return ScopedArray<Type>(static_cast<Type*>(ptr));
}

XAMP_BASE_NAMESPACE_END

