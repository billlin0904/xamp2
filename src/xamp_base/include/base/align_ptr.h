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

XAMP_BASE_API void* StackAlloc(size_t size);

XAMP_BASE_API void StackFree(void* p);

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
using StackBufferPtr = std::unique_ptr<Type[], StackBufferDeleter<Type>>;

template <typename Type>
using AlignPtr = std::unique_ptr<Type, AlignedClassDeleter<Type>>;

template <typename Type>
using AlignBufferPtr = std::unique_ptr<Type[], AlignedDeleter<Type>>;

template <typename BaseType, typename ImplType, typename... Args>
XAMP_BASE_API_ONLY_EXPORT AlignPtr<BaseType> MakeAlign(Args&& ... args) {
    auto ptr = AlignedMallocOf<ImplType>(kMallocAlignSize);

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

template <typename Type, typename... Args>
XAMP_BASE_API_ONLY_EXPORT AlignPtr<Type> MakeAlign(Args&& ... args) {
    auto ptr = AlignedMallocOf<Type>(kMallocAlignSize);
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

template <typename Type, typename U = std::enable_if_t<std::is_trivially_copyable<Type>::value>>
XAMP_BASE_API_ONLY_EXPORT AlignBufferPtr<Type> MakeBuffer(size_t n, size_t alignment = kMallocAlignSize) {
    auto ptr = AlignedMallocCountOf<Type>(n, alignment);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return AlignBufferPtr<Type>(static_cast<Type*>(ptr));
}

template <typename Type, typename U = std::enable_if_t<std::is_trivially_copyable<Type>::value>>
XAMP_BASE_API_ONLY_EXPORT StackBufferPtr<Type> MakeStackBuffer(size_t n) {
    auto ptr = StackAlloc(sizeof(Type) * n);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return StackBufferPtr<Type>(static_cast<Type*>(ptr));
}

template <typename T>
class XAMP_BASE_API_ONLY_EXPORT Buffer {
public:
    Buffer() = default;

    explicit Buffer(size_t size)
        : ptr_(MakeBuffer<T>(size))
        , size_(size) {
    }

    Buffer(const Buffer& other)
        : Buffer(other.size_) {
        std::memcpy(ptr_.get(), other.ptr_.get(), size_ * sizeof(T));
    }

    Buffer(Buffer&&) = default;

    Buffer& operator=(const Buffer& other) {
        Buffer copy(other);
        swap(*this, copy);
        return *this;
    }

    Buffer& operator=(Buffer&&) = default;

    friend void swap(Buffer& a, Buffer& b) noexcept {
        std::swap(a.ptr_, b.ptr_);
        std::swap(a.size_, b.size_);
    }

    T* Get() noexcept { 
        return ptr_.get();
    }    

    const T* Get() const noexcept {
        return ptr_.get();
    }

    const T* data() const noexcept {
        return ptr_.get();
    }

    [[nodiscard]] size_t GetSize() const noexcept {
        return size_;
    }

    [[nodiscard]] size_t GetByteSize() const noexcept {
        return size_ * sizeof(T);
    }

    // 兼容STL容器相關函數.

    T* data() noexcept {
        return ptr_.get();
    }

    T& operator[](size_t i) noexcept {
        return ptr_[i]; 
    }

    const T& operator[](size_t i) const noexcept {
        return ptr_[i]; 
    }

    [[nodiscard]] size_t size() const noexcept {
        return size_;
    }

    void resize(size_t new_size) {
        clear();
        *this = Buffer<T>(new_size);
    }

    void clear() {
        ptr_.reset();
        size_ = 0;
    }

private:
    AlignBufferPtr<T> ptr_;
    size_t size_ = 0;
};


}

