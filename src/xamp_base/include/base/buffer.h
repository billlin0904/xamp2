//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/align_ptr.h>
#include <base/vmmemlock.h>
#include <base/assert.h>

XAMP_BASE_NAMESPACE_BEGIN

/*
* Buffer<T> is a wrapper of std::unique_ptr<T[]>.
* 
* @tparam T Type of buffer.
* @tparam U Enable if T is trivially copyable.
*/
template <typename T, typename U = std::enable_if_t<std::is_trivially_copyable_v<T>>>
class XAMP_BASE_API_ONLY_EXPORT Buffer {
public:
    Buffer() = default;

    explicit Buffer(const size_t size)
        : size_(size)
        , ptr_(MakeAlignedArray<T>(size)) {
        lock_.Lock(ptr_.get(), GetByteSize());
    }

	XAMP_DISABLE_COPY(Buffer)

    Buffer(Buffer<T, U>&& other) noexcept {
        *this = std::move(other);
    }

    Buffer<T, U>& operator=(Buffer<T, U>&& other) noexcept {
        if (this != &other) {
            ptr_ = std::move(other.ptr_);
            lock_ = std::move(other.lock_);
            size_ = other.size_;
            other.size_ = 0;
        }
        return *this;
    }

    T* Get() noexcept { 
        return ptr_.get();
    }

    [[nodiscard]] const T* Get() const noexcept {
        return ptr_.get();
    }

    [[nodiscard]] const T* data() const noexcept {
        return ptr_.get();
    }

    [[nodiscard]] size_t GetSize() const noexcept {
        return size_;
    }

    [[nodiscard]] size_t GetByteSize() const noexcept {
        return size_ * sizeof(T);
    }

    void Fill(T value) {
        MemorySet(get(), value, GetByteSize());
    }

    // 兼容STL容器相關函數.

    T* data() noexcept {
        return ptr_.get();
    }

    T* get() noexcept {
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
        if (new_size != size_) {
            Buffer<T> new_buf(new_size);
            MemoryCopy(new_buf.data(), ptr_.get(), (std::min)(new_size, size_));
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
    AlignArray<T> ptr_;
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
    explicit BufferRef(Buffer<T>& buf)
        : buffer_(buf.get())
        , size_(buf.size())
		, ref_(buf) {
    }

    void maybe_resize(size_t size) noexcept {
        if (size > ref_.size()) {
            ref_.resize(size);
            buffer_ = ref_.get();
        }
        size_ = size;
    }

    T* data() noexcept {
        return buffer_;
    }

    const T* data() const noexcept {
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