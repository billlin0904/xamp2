//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/assert.h>

XAMP_BASE_NAMESPACE_BEGIN

// CopyFrom from book 'The Modern Cpp Challenge'

template <typename T>
class CircularBuffer;

/*
* CircularBufferIterator is an iterator for a circular buffer.
* 
* @param[in] T The type of the elements in the buffer.
*/
template <typename T>
class CircularBufferIterator {
    typedef CircularBufferIterator self_type;
    typedef T value_type;
    typedef T& reference;
    typedef T const& const_reference;
    typedef T* pointer;
    typedef std::random_access_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
public:
    /*
    * Constructor.
    */
    CircularBufferIterator(CircularBuffer<T> const& buf, size_t const pos, bool const last)
        : buffer_(buf)
        , index_(pos)
        , last_(last) {
    }

    /*
    * Operator ++.
    * 
    * @return A reference to the iterator after incrementing it.
    */
    self_type& operator++ () {
        if (last_) {
            throw std::out_of_range("Iterator cannot be incremented past the end of range.");
        }
        index_ = (index_ + 1) % buffer_.data_.size();
        last_ = index_ == buffer_.next_pos();
        return *this;
    }

    /*
    * Operator + n.
    * 
    * @return A reference to the iterator after incrementing it.
    */
    self_type operator++ (int) {
        self_type tmp = *this;
        ++* this;
        return tmp;
    }

    /*
    * Operator ==.
    * 
    * @return true if the iterators are equal, false otherwise.
    */
    bool operator== (self_type const& other) const {
        XAMP_EXPECTS(compatible(other));
        return index_ == other.index_ && last_ == other.last_;
    }

    /*
    * Operator !=.
    * 
    * @return true if the iterators are not equal, false otherwise.
    */
    bool operator!= (self_type const& other) const {
        return !(*this == other);
    }

    /*
    * Operator *.
    * 
    * @return A reference to the element pointed to by the iterator.    
    */
    const_reference operator* () const {
        return buffer_.data_[index_];
    }

    /*
    * Operator ->.
    * 
    * @return A pointer to the element pointed to by the iterator.     
    */
    const_reference operator-> () const {
        return buffer_.data_[index_];
    }

private:
    /*
    * Check if the iterators are compatible.
    * @return true if the iterators are compatible, false otherwise.
    */
    bool compatible(self_type const& other) const {
        return &buffer_ == &other.buffer_;
    }

    CircularBuffer<T> const& buffer_;
    size_t index_;
    bool last_;
};

/*
* CircularBuffer is a circular buffer.
* 
* The buffer is implemented using a vector.
* The buffer is bounded, and the size is specified in the constructor.
* The buffer is not thread-safe, and can only be used by one producer and one consumer.
* 
* @param[in] T The type of the elements in the buffer.
*/
template <typename T>
class CircularBuffer {
    typedef CircularBufferIterator<T> const_iterator;

    CircularBuffer() = delete;
public:
    /*
    * Constructor.
    * 
    * @param[in] size The size of the buffer.     
    */
    explicit CircularBuffer(size_t size)
        : data_(size) {
    }

    /*
    * Clear the buffer.
    * 
    * After this call, the buffer is empty.    
    */
    void clear() noexcept { 
        head_ = -1; size_ = 0;
    }

    /*
    * Check if the buffer is empty.
    * 
    * @return true if the buffer is empty, false otherwise.     
    */
    XAMP_NO_DISCARD bool empty() const noexcept {
        return size_ == 0;
    }

    /*
    * Check if the buffer is full.
    * 
    * @return true if the buffer is full, false otherwise.
    */
    XAMP_NO_DISCARD bool full() const noexcept {
        return size_ == data_.size();
    }

    /*
    * Get the capacity of the buffer.
    * 
    * @return The capacity of the buffer.     
    */
    XAMP_NO_DISCARD size_t capacity() const noexcept { 
        return data_.size();
    }

    /*
    * Get the size of the buffer.
    * 
    * @return The size of the buffer.
    */
    XAMP_NO_DISCARD size_t size() const noexcept { 
        return size_;
    }

    /*
    * Add an item to the buffer.
    * 
    * @param[in] item The item to add to the buffer.    
    */
    void emplace_back(T&& item) {
        head_ = next_pos();
        data_[head_] = std::move(item);

        if (size_ < data_.size()) {
            size_++;
        }
    }

    /*
    * Add an item to the buffer.
    * 
    * @param[in] item The item to add to the buffer.
    */
    void push(const T & item) {
        head_ = next_pos();
        data_[head_] = item;

        if (size_ < data_.size()) {
            size_++;
        }
    }

    /*
     * Get the front item in the buffer.
     */
    XAMP_NO_DISCARD T& front() {
        if (empty()) {
            throw std::runtime_error("empty buffer");
        }
        return data_[first_pos()];
    }

    /*
	 * Get the front item in the buffer.
	 */
    XAMP_NO_DISCARD const T& front() const {
        if (empty()) {
            throw std::runtime_error("empty buffer");
        }
        return data_[first_pos()];
    }

    /*
	 * Remove an item from the buffer.
	 */
    void pop_front() {
        if (empty()) {
            throw std::runtime_error("empty buffer");
        }
        size_--;
    }

    /*
    * Begin iterator.
    * 
    * @return The begin iterator.
    */
    XAMP_NO_DISCARD const_iterator begin() const {
        return const_iterator(*this, first_pos(), empty());
    }

    /*
    * End iterator.
    * 
    * @return The end iterator.
    */
    XAMP_NO_DISCARD const_iterator end() const {
        return const_iterator(*this, next_pos(), true);
    }

private:
    size_t head_ = -1;
    size_t size_ = 0;
    std::vector<T> data_;

    /*
    * Get the next position.
    * 
    * @return The next position.
    */
    XAMP_NO_DISCARD size_t next_pos() const noexcept {
        return size_ == 0 ? 0 : (head_ + 1) % data_.size(); 
    }

    /*
    * Get the first position.
    * 
    * @return The first position.
    */
    XAMP_NO_DISCARD size_t first_pos() const noexcept {
        return size_ == 0 ? 0 : (head_ + data_.size() - size_ + 1) % data_.size(); 
    }

    friend class CircularBufferIterator<T>;
};

XAMP_BASE_NAMESPACE_END