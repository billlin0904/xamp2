//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <cassert>

#include <base/base.h>

namespace xamp::base {

XAMP_BASE_API void* AlignedAlloc(size_t size, size_t aligned_size);
XAMP_BASE_API void AlignedFree(void* p);

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
using AlignPtr = std::unique_ptr<Type, AlignedClassDeleter<Type>>;

template <typename Type>
using AlignBufferPtr = std::unique_ptr<Type[], AlignedDeleter<Type>>;

template <typename BaseType, typename ImplType, typename... Args>
XAMP_BASE_API_ONLY_EXPORT AlignPtr<BaseType> MakeAlign(Args&& ... args) {
    auto ptr = AlignedAlloc(sizeof(ImplType), XAMP_MALLOC_ALGIGN_SIZE);

    if (!ptr) {
        throw std::bad_alloc();
    }

    assert(((size_t)ptr % XAMP_MALLOC_ALGIGN_SIZE) == 0);

    try {
        BaseType* base = ::new(ptr) ImplType(std::forward<Args>(args)...);
        return AlignPtr<BaseType>(base);
    }
    catch (...) {
        AlignedFree(ptr);
        throw;
    }
}

template <typename Type, typename... Args>
XAMP_BASE_API_ONLY_EXPORT AlignPtr<Type> MakeAlign(Args&& ... args) {
    auto ptr = AlignedAlloc(sizeof(Type), XAMP_MALLOC_ALGIGN_SIZE);
    if (!ptr) {
        throw std::bad_alloc();
    }

    assert(((size_t)ptr % XAMP_MALLOC_ALGIGN_SIZE) == 0);

    try {
        auto q = ::new(ptr) Type(std::forward<Args>(args)...);
        return AlignPtr<Type>(q);
    }
    catch (...) {
        AlignedFree(ptr);
        throw;
    }
}

template <typename Type, typename U = std::enable_if_t<std::is_trivially_copyable<Type>::value>>
XAMP_BASE_API_ONLY_EXPORT AlignBufferPtr<Type> MakeBuffer(size_t size, const int32_t alignment = XAMP_MALLOC_ALGIGN_SIZE) {
    auto ptr = AlignedAlloc(size * sizeof(Type), alignment);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return AlignBufferPtr<Type>(static_cast<Type*>(ptr));
}

}

