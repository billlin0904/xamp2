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
#ifdef XAMP_OS_WIN
        _freea(p);
#else
        (void)p;
#endif
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
#ifdef XAMP_OS_MAC
    auto ptr = alloca(sizeof(Type) * n);
#else
    auto ptr =  _malloca(sizeof(Type) * n);
#endif
    if (!ptr) {
        throw std::bad_alloc();
    }
    return StackBufferPtr<Type>(static_cast<Type*>(ptr));
}

template <typename T>
class XAMP_BASE_API_ONLY_EXPORT AlignedBuffer {
public:
    AlignedBuffer() = default;

    explicit AlignedBuffer(size_t size)
        : ptr_(MakeBuffer<T>(size)), size_(size) {
    }

    AlignedBuffer(const AlignedBuffer& other)
        : AlignedBuffer(other.size_) {
        std::memcpy(ptr_.get(), other.ptr_.get(), size_ * sizeof(T));
    }

    AlignedBuffer(AlignedBuffer&&) = default;

    AlignedBuffer& operator=(const AlignedBuffer& other) {
        AlignedBuffer copy(other);
        swap(*this, copy);
        return *this;
    }

    AlignedBuffer& operator=(AlignedBuffer&&) = default;

    friend void swap(AlignedBuffer& a, AlignedBuffer& b) noexcept {
        std::swap(a.ptr_, b.ptr_);
        std::swap(a.size_, b.size_);
    }

    T* Get() noexcept { 
        return ptr_.get();
    }

    const T* Get() const noexcept {
        return ptr_.get();
    }

    T& operator[](size_t i) noexcept {
        return ptr_[i]; 
    }

    const T& operator[](size_t i) const noexcept {
        return ptr_[i]; 
    }

    [[nodiscard]] size_t GetSize() const noexcept {
        return size_;
    }

    [[nodiscard]] size_t GetByteSize() const noexcept {
        return size_ * sizeof(T);
    }

private:
    AlignBufferPtr<T> ptr_;
    size_t size_ = 0;
};


}

