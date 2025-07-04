﻿//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/vmmemlock.h>
#include <base/assert.h>

XAMP_BASE_NAMESPACE_BEGIN

/*
* Buffer<T> is a wrapper of std::unique_ptr<T[]>.
* 
* @tparam T Type of buffer.
* @tparam U Enable if T is trivially copyable.
*/
template <typename T>
class XAMP_BASE_API_ONLY_EXPORT Buffer {
public:
    static_assert(std::is_trivially_copyable_v<T>, "Buffer only supports trivially copyable types.");

    Buffer() = default;

    explicit Buffer(const size_t size)
        : size_(size)
        , ptr_(MakeAlignedArray<T>(size)) {
        lock_.Lock(ptr_.get(), GetByteSize());
    }

	XAMP_DISABLE_COPY(Buffer)

    Buffer(Buffer<T>&& other) noexcept {
        *this = std::move(other);
    }

    Buffer<T>& operator=(Buffer<T>&& other) noexcept {
        if (this != &other) {
            ptr_ = std::move(other.ptr_);
            lock_ = std::move(other.lock_);
            size_ = other.size_;
            other.size_ = 0;
        }
        return *this;
    }

    [[nodiscard]] T* Get() noexcept XAMP_CHECK_LIFETIME {
        return ptr_.get();
    }

    [[nodiscard]] const T* Get() const noexcept XAMP_CHECK_LIFETIME {
        return ptr_.get();
    }

    [[nodiscard]] const T* data() const noexcept XAMP_CHECK_LIFETIME {
        return ptr_.get();
    }

    [[nodiscard]] size_t GetSize() const noexcept {
        return size_;
    }

    [[nodiscard]] size_t GetByteSize() const noexcept {
        return size_ * sizeof(T);
    }

    void Fill(T value) {
        std::fill(ptr_.get(), ptr_.get() + size_, value);
    }

    // 兼容STL容器相關函數.

    [[nodiscard]] T* data() noexcept XAMP_CHECK_LIFETIME {
        return ptr_.get();
    }

    [[nodiscard]] T* get() noexcept XAMP_CHECK_LIFETIME {
        return ptr_.get();
    }

    [[nodiscard]] T& operator[](size_t i) noexcept XAMP_CHECK_LIFETIME {
        return ptr_[i]; 
    }

    [[nodiscard]] const T& operator[](size_t i) const noexcept XAMP_CHECK_LIFETIME {
        return ptr_[i]; 
    }

    [[nodiscard]] size_t size() const noexcept {
        return size_;
    }

    void resize(size_t new_size) {
        if (new_size == size_) {
            return;
        }
        else if (new_size < size_) {
            size_ = new_size;
        }
        else {
            Buffer<T> new_buf(new_size);
            MemoryCopy(new_buf.data(), ptr_.get(), size_ * sizeof(T));
            *this = std::move(new_buf);
        }
    }

    void reset() {
        lock_.UnLock();
        ptr_.reset();
        size_ = 0;
    }

private:
    size_t size_ = 0;
    ScopedArray<T> ptr_;
    VmMemLock lock_;
};

/*
* BufferRef<T> is a wrapper of Buffer<T>.
* 
* @tparam T Type of buffer.
* @tparam U Enable if T is trivially copyable.
* @note BufferRef<T> is not thread safe.
*/
template <typename T, typename U = std::enable_if_t<std::is_trivially_copyable_v<T>>>
struct XAMP_BASE_API_ONLY_EXPORT BufferRef {
    using value_type = T;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    explicit BufferRef(Buffer<T>& buf)
        : buffer_(buf.get())
        , size_(buf.size())
		, ref_(buf) {
    }

    XAMP_DISABLE_COPY_AND_MOVE(BufferRef)

    void CopyFrom(const T *buffer, size_t buffer_size) {
        XAMP_ENSURES(buffer != nullptr);
        XAMP_ENSURES(buffer_size <= size_);
        if (buffer_size > 0) {
            MemoryCopy(data(), buffer, buffer_size * sizeof(T));
        }
    }

    template <typename InputIt>
    void CopyFrom(InputIt first, InputIt last) {
        const auto count = std::distance(first, last);
        if (count > 0) {
            CopyFrom(&(*first), static_cast<size_type>(count));
        }
    }

    void maybe_resize(size_t size) noexcept {
        if (size > ref_.size()) {
            ref_.resize(size);
            buffer_ = ref_.get();
        }
        size_ = size;
    }

    [[nodiscard]] reference operator[](size_type pos) noexcept XAMP_CHECK_LIFETIME {
        return buffer_[pos];
    }

    [[nodiscard]] const_reference operator[](size_type pos) const noexcept XAMP_CHECK_LIFETIME {
        return buffer_[pos];
    }

    [[nodiscard]] reference at(size_type pos) XAMP_CHECK_LIFETIME {
        if (pos >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return buffer_[pos];
    }

    [[nodiscard]] const_reference at(size_type pos) const XAMP_CHECK_LIFETIME {
        if (pos >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return buffer_[pos];
    }

    // Iterators
    iterator begin() noexcept {
	    return buffer_;
    }

    const_iterator begin() const noexcept {
	    return buffer_;
    }

    const_iterator cbegin() const noexcept {
	    return buffer_;
    }

    iterator end() noexcept {
	    return buffer_ + size_;
    }

    const_iterator end() const noexcept {
	    return buffer_ + size_;
    }

    const_iterator cend() const noexcept {
        return buffer_ + size_;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0;
    }

    [[nodiscard]] pointer data() noexcept XAMP_CHECK_LIFETIME {
        return buffer_;
    }

    [[nodiscard]] const_pointer data() const noexcept XAMP_CHECK_LIFETIME {
        return buffer_;
    }

    [[nodiscard]] size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] size_t GetByteSize() const noexcept {
        return size_ * sizeof(T);
    }

private:
    T* buffer_;
    size_t size_;
    Buffer<T>& ref_;
};

template <typename T>
XAMP_BASE_API_ONLY_EXPORT Buffer<T> MakeBuffer(size_t size) {
    XAMP_ENSURES(size > 0);
    return Buffer<T>(size);
}

XAMP_BASE_NAMESPACE_END