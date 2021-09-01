//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <base/vmmemlock.h>

namespace xamp::base {

template <typename T, typename U = std::enable_if_t<std::is_trivially_copyable_v<T>>>
class XAMP_BASE_API_ONLY_EXPORT Buffer {
public:
    Buffer() = default;

    explicit Buffer(const size_t size)
        : size_(size)
        , ptr_(MakeBufferPtr<T>(size)) {
        lock_.Lock(ptr_.get(), GetByteSize());
    }

	XAMP_DISABLE_COPY(Buffer)

    Buffer(Buffer&& other) noexcept {
        *this = std::move(other);
    }

    Buffer& operator=(Buffer&& other) noexcept {
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
    AlignBufferPtr<T> ptr_;    
    VmMemLock lock_;
};

template <typename T>
XAMP_BASE_API_ONLY_EXPORT Buffer<T> MakeBuffer(size_t size) {
    return Buffer<T>(size);
}

}


