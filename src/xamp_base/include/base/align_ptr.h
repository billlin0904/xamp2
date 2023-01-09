//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <cassert>
#include <base/base.h>

namespace xamp::base {

XAMP_BASE_API void* AlignedMalloc(size_t size, size_t aligned_size) noexcept;

XAMP_BASE_API void AlignedFree(void* p) noexcept;

XAMP_BASE_API void* StackAlloc(size_t size);

XAMP_BASE_API void StackFree(void* p);

template <typename T>
constexpr T AlignUp(T value, size_t aligned_size = kMallocAlignSize) {
    return T((value + (T(aligned_size) - 1)) & ~T(aligned_size - 1));
}

template <typename Type>
XAMP_BASE_API_ONLY_EXPORT Type* AlignedMallocOf(size_t aligned_size) noexcept {
    return static_cast<Type*>(AlignedMalloc(sizeof(Type), aligned_size));
}

template <typename Type>
XAMP_BASE_API_ONLY_EXPORT Type* AlignedMallocCountOf(size_t n, size_t aligned_size) noexcept {
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
    void operator()(Type* p) const noexcept {
        p->~Type();
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

using CharPtr = std::unique_ptr<char, FreeDeleter<char>>;

template <typename Type>
using StackBufferPtr = std::unique_ptr<Type[], StackBufferDeleter<Type>>;

template <typename Type>
using AlignPtr = std::unique_ptr<Type, AlignedClassDeleter<Type>>;

template <typename Type>
using AlignArray = std::unique_ptr<Type[], AlignedDeleter<Type>>;

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

template <typename Type>
XAMP_BASE_API_ONLY_EXPORT AlignArray<Type> MakeAlignedArray(size_t n) {
    auto ptr = AlignedMallocCountOf<Type>(n, kMallocAlignSize);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return AlignArray<Type>(static_cast<Type*>(ptr));
}

template <typename Type = uint8_t>
XAMP_BASE_API_ONLY_EXPORT StackBufferPtr<Type> MakeStackBuffer(size_t n) {
    auto* ptr = StackAlloc(sizeof(Type) * n);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return StackBufferPtr<Type>(static_cast<Type*>(ptr));
}

}

