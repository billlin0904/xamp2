//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/vmmemlock.h>
#include <base/str_utilts.h>
#include <base/assert.h>

XAMP_BASE_NAMESPACE_BEGIN

/*
* Buffer<t> is a wrapper of std::unique_ptr<t[]>.
* 
* @tparam t Type of buffer.
* @tparam U Enable if t is trivially copyable.
*/
template <typename t>
class Buffer {
public:
    static_assert(std::is_trivially_copyable_v<t>, "Buffer only supports trivially copyable types.");

    Buffer() = default;

    explicit Buffer(const size_t size)
        : size_(size)
        , ptr_(makeAlignedArray<t>(size)) {
        lock_.Lock(ptr_.get(), getByteSize());
    }

	XAMP_DISABLE_COPY(Buffer)

    Buffer(Buffer<t>&& other) {
        *this = std::move(other);
    }

    Buffer<t>& operator=(Buffer<t>&& other) {
        if (this != &other) {
            ptr_ = std::move(other.ptr_);
            lock_ = std::move(other.lock_);
            size_ = other.size_;
            other.size_ = 0;
        }
        return *this;
    }

    [[nodiscard]] const t* data() const XAMP_CHECK_LIFETIME {
        return ptr_.get();
    }

    [[nodiscard]] size_t getSize() const {
        return size_;
    }

    [[nodiscard]] size_t getByteSize() const {
        return size_ * sizeof(t);
    }

    [[nodiscard]] std::string getByteSizeString() const {
        return String::FormatBytesBy<t>(getByteSize());
    }

    void Fill(t value) {
        std::fill(ptr_.get(), ptr_.get() + size_, value);
    }

    // 兼容STL容器相關函數.

    [[nodiscard]] t* data() XAMP_CHECK_LIFETIME {
        return ptr_.get();
    }

    [[nodiscard]] t* get() XAMP_CHECK_LIFETIME {
        return ptr_.get();
    }

    [[nodiscard]] t& operator[](size_t i) XAMP_CHECK_LIFETIME {
        return ptr_[i]; 
    }

    [[nodiscard]] const t& operator[](size_t i) const XAMP_CHECK_LIFETIME {
        return ptr_[i]; 
    }

    [[nodiscard]] size_t size() const {
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
            Buffer<t> new_buf(new_size);
            MemoryCopy(new_buf.data(), ptr_.get(), size_ * sizeof(t));
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
    ScopedArray<t> ptr_;
    VmMemLock lock_;
};

/*
* BufferRef<t> is a wrapper of Buffer<t>.
* 
* @tparam t Type of buffer.
* @tparam U Enable if t is trivially copyable.
* @note BufferRef<t> is not thread safe.
*/
template <typename t, typename U = std::enable_if_t<std::is_trivially_copyable_v<t>>>
struct BufferRef {
    using value_type = t;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    explicit BufferRef(Buffer<t>& buf)
        : buffer_(buf.get())
        , size_(buf.size())
		, ref_(buf) {
    }

    XAMP_DISABLE_COPY_AND_MOVE(BufferRef)

    void CopyFrom(const t *buffer, size_t buffer_size) {
        XAMP_ENSURES(buffer != nullptr);
        XAMP_ENSURES(buffer_size <= size_);
        if (buffer_size > 0) {
            MemoryCopy(data(), buffer, buffer_size * sizeof(t));
        }
    }

    template <typename InputIt>
    void copyFrom(InputIt first, InputIt last) {
        const auto count = std::distance(first, last);
        if (count > 0) {
            copyFrom(&(*first), static_cast<size_type>(count));
        }
    }

    void maybe_resize(size_t size) {
        if (size > ref_.size()) {
            ref_.resize(size);
            buffer_ = ref_.get();
        }
        size_ = size;
    }

    [[nodiscard]] reference operator[](size_type pos) XAMP_CHECK_LIFETIME {
        return buffer_[pos];
    }

    [[nodiscard]] const_reference operator[](size_type pos) const XAMP_CHECK_LIFETIME {
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
    iterator begin() {
	    return buffer_;
    }

    const_iterator begin() const {
	    return buffer_;
    }

    const_iterator cbegin() const {
	    return buffer_;
    }

    iterator end() {
	    return buffer_ + size_;
    }

    const_iterator end() const {
	    return buffer_ + size_;
    }

    const_iterator cend() const {
        return buffer_ + size_;
    }

    [[nodiscard]] bool empty() const {
        return size_ == 0;
    }

    [[nodiscard]] pointer data() XAMP_CHECK_LIFETIME {
        return buffer_;
    }

    [[nodiscard]] const_pointer data() const XAMP_CHECK_LIFETIME {
        return buffer_;
    }

    [[nodiscard]] size_t size() const {
        return size_;
    }

    [[nodiscard]] size_t getByteSize() const {
        return size_ * sizeof(t);
    }

private:
    t* buffer_;
    size_t size_;
    Buffer<t>& ref_;
};

template <typename t>
Buffer<t> makeBuffer(size_t size) {
    XAMP_ENSURES(size > 0);
    return Buffer<t>(size);
}

XAMP_BASE_NAMESPACE_END