//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <cassert>

#include <base/base.h>

namespace xamp::base {

XAMP_BASE_API void* AlignedMalloc(size_t size, size_t aligned_size) noexcept;

XAMP_BASE_API void AlignedFree(void* p) noexcept;

template <typename Type>
XAMP_BASE_API_ONLY_EXPORT Type* AlignedMallocOf(size_t aligned_size) noexcept {
    return reinterpret_cast<Type*>(AlignedMalloc(sizeof(Type), aligned_size));
}

template <typename Type>
XAMP_BASE_API_ONLY_EXPORT Type* AlignedMallocCountOf(size_t n, size_t aligned_size) noexcept {
    return reinterpret_cast<Type*>(AlignedMalloc(sizeof(Type) * n, aligned_size));
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
        p->~Type();
        AlignedFree(p);
    }
};

template <typename Type>
using align_ptr = std::unique_ptr<Type, AlignedClassDeleter<Type>>;

template <typename Type>
using align_buffer_ptr = std::unique_ptr<Type[], AlignedDeleter<Type>>;

template <typename BaseType, typename ImplType, typename... Args>
XAMP_BASE_API_ONLY_EXPORT align_ptr<BaseType> MakeAlign(Args&& ... args) {
    auto ptr = AlignedMallocOf<ImplType>(XAMP_MALLOC_ALGIGN_SIZE);

    if (!ptr) {
        throw std::bad_alloc();
    }

    assert(((size_t)ptr % XAMP_MALLOC_ALGIGN_SIZE) == 0);

    try {
        BaseType* base = ::new(ptr) ImplType(std::forward<Args>(args)...);
        return align_ptr<BaseType>(base);
    }
    catch (...) {
        AlignedFree(ptr);
        throw;
    }
}

template <typename Type, typename... Args>
XAMP_BASE_API_ONLY_EXPORT align_ptr<Type> MakeAlign(Args&& ... args) {
    auto ptr = AlignedMallocOf<Type>(XAMP_MALLOC_ALGIGN_SIZE);
    if (!ptr) {
        throw std::bad_alloc();
    }

    assert(((size_t)ptr % XAMP_MALLOC_ALGIGN_SIZE) == 0);

    try {
        auto q = ::new(ptr) Type(std::forward<Args>(args)...);
        return align_ptr<Type>(q);
    }
    catch (...) {
        AlignedFree(ptr);
        throw;
    }
}

template <typename Type, typename U = std::enable_if_t<std::is_trivially_copyable<Type>::value>>
XAMP_BASE_API_ONLY_EXPORT align_buffer_ptr<Type> MakeBuffer(size_t n, const int32_t alignment = XAMP_MALLOC_ALGIGN_SIZE) {
    auto ptr = AlignedMallocCountOf<Type>(n, alignment);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return align_buffer_ptr<Type>(static_cast<Type*>(ptr));
}

}

