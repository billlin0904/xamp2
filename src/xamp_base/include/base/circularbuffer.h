//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

namespace xamp::base {

// Copy from book 'The Modern Cpp Challenge'

template <class T>
class CircularBuffer;

template <class T>
class CircularBufferIterator {
    typedef CircularBufferIterator self_type;
    typedef T value_type;
    typedef T& reference;
    typedef T const& const_reference;
    typedef T* pointer;
    typedef std::random_access_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
public:
    CircularBufferIterator(CircularBuffer<T> const& buf, size_t const pos, bool const last)
        : buffer_(buf)
        , index_(pos)
        , last_(last) {
    }

    self_type& operator++ () {
        if (last_) {
            throw std::out_of_range("Iterator cannot be incremented past the end of range.");
        }
        index_ = (index_ + 1) % buffer_.data_.size();
        last_ = index_ == buffer_.next_pos();
        return *this;
    }

    self_type operator++ (int) {
        self_type tmp = *this;
        ++* this;
        return tmp;
    }

    bool operator== (self_type const& other) const {
        assert(compatible(other));
        return index_ == other.index_ && last_ == other.last_;
    }

    bool operator!= (self_type const& other) const {
        return !(*this == other);
    }

    const_reference operator* () const {
        return buffer_.data_[index_];
    }

    const_reference operator-> () const {
        return buffer_.data_[index_];
    }

private:
    bool compatible(self_type const& other) const {
        return &buffer_ == &other.buffer_;
    }

    CircularBuffer<T> const& buffer_;
    size_t index_;
    bool last_;
};

template <class T>
class CircularBuffer {
    typedef CircularBufferIterator<T> const_iterator;

    CircularBuffer() = delete;
public:
    explicit CircularBuffer(size_t size)
        : data_(size) {
    }

    void clear() noexcept { 
        head_ = -1; size_ = 0;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0;
    }

    [[nodiscard]] bool full() const noexcept {
        return size_ == data_.size();
    }

    [[nodiscard]] size_t capacity() const noexcept { 
        return data_.size();
    }

    [[nodiscard]] size_t size() const noexcept { 
        return size_;
    }

    void emplace(T&& item) {
        head_ = next_pos();
        data_[head_] = std::move(item);

        if (size_ < data_.size()) {
            size_++;
        }
    }

    void push(const T item) {
        head_ = next_pos();
        data_[head_] = item;

        if (size_ < data_.size()) {
            size_++;
        }
    }

    T& pop() noexcept {
        if (empty()) {
            throw std::runtime_error("empty buffer");
        }

        auto pos = first_pos();
        size_--;
        return data_[pos];
    }

    [[nodiscard]] const_iterator begin() const {
        return const_iterator(*this, first_pos(), empty());
    }

    [[nodiscard]] const_iterator end() const {
        return const_iterator(*this, next_pos(), true);
    }

private:
    size_t head_ = -1;
    size_t size_ = 0;
    std::vector<T> data_;

    [[nodiscard]] size_t next_pos() const noexcept {
        return size_ == 0 ? 0 : (head_ + 1) % data_.size();
    }

    [[nodiscard]] size_t first_pos() const noexcept {
        return size_ == 0 ? 0 : (head_ + data_.size() - size_ + 1) % data_.size();
    }

    friend class CircularBufferIterator<T>;
};

}
