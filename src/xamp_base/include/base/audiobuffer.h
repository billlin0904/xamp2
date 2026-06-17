//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>

#include <base/base.h>
#include <base/memory.h>
#include <base/buffer.h>

XAMP_BASE_NAMESPACE_BEGIN
/*
* AudioBuffer is a thread safe circular buffer.
* 
* @tparam t is the type of the buffer.
* @tparam U is the enable_if_t type.
*/
template
<
	typename t,
	typename U = std::enable_if_t<std::is_trivially_copyable_v<t>>>
class AudioBuffer final {
public:
	/*
	* Constructor.
	*/
	AudioBuffer() ;

	/*
	* Constructor.
	*/
	explicit AudioBuffer(size_t size);

	/*
	* Destructor.
	*/
	~AudioBuffer() ;

	XAMP_DISABLE_COPY_AND_MOVE(AudioBuffer)

	/*
	* Get the buffer data.
	* 
	* @return the buffer data.
	*/
	t* getData() const ;

	/*
	* Clear the buffer.
	* 
	*/
	void clear() ;

	/*
	* Resize the buffer.
	* 
	* @param[in] size is the new size of the buffer.
	*/
	void resize(size_t size);

	/*
	* Get the buffer size.
	* 
	* @return the buffer size.
	*/
	size_t getSize() const ;

	/*
	* Get the buffer byte size.
	* 
	* @return the buffer byte size.
	*/
	size_t getByteSize() const ;

	/*
	* Get the available write size.
	* 
	* @return the available write size.
	*/
	size_t getAvailableWrite() const ;

	/*
	* Get the available read size.
	* 
	* @return the available read size.
	*/
	size_t getAvailableRead() const ;

	/*
	* write data to the buffer.
	* 
	* @param[in] data is the data to write.
	* @param[in] count is the number of data to write.
	* @return true if write success.
	*/
	bool tryWrite(const t* data, size_t count) ;

	/*
	* read data from the buffer.
	* 
	* @param[out] data is the data to read.
	* @param[in] count is the number of data to read.
	* @param[out] num_filled_count is the number of data read.
	* @return true if read success.
	*/
	bool tryRead(t* data, size_t count, size_t& num_filled_count) ;

	/*
	* Fill the buffer with value.
	* 
	* @param[in] value is the value to fill.
	*/
	void fill(t value) ;

private:
	/*
	* Get the available write size.
	* 
	* @param[in] head is the head of the buffer.
	* @param[in] tail is the tail of the buffer.
	* @return the available write size.
	*/
	size_t getAvailableWrite(size_t head, size_t tail) const ;

	/*
	* Get the available read size.
	* 
	* @param[in] head is the head of the buffer.
	* @param[in] tail is the tail of the buffer.
	* @return the available read size.
	*/
	size_t getAvailableRead(size_t head, size_t tail) const ;

	XAMP_CACHE_ALIGNED(kCacheAlignSize) size_t size_;
	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<size_t> head_;
	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<size_t> tail_;
	Buffer<t> buffer_;
};

template <typename Type, typename U>
AudioBuffer<Type, U>::AudioBuffer() : size_(0)
	, head_(0)
	, tail_(0) {
}

template <typename Type, typename U>
AudioBuffer<Type, U>::AudioBuffer(size_t size)
	: AudioBuffer() {
	resize(size);
}

template <typename Type, typename U>
AudioBuffer<Type, U>::~AudioBuffer() = default;

template <typename Type, typename U>
Type* AudioBuffer<Type, U>::getData() const {
	return buffer_.get();
}

template <typename Type, typename U>
size_t AudioBuffer<Type, U>::getSize() const {
	return size_;
}

template <typename Type, typename U>
size_t AudioBuffer<Type, U>::getByteSize() const {
	return getSize() * 8;
}

template <typename Type, typename U>
void AudioBuffer<Type, U>::resize(size_t size) {
	if (size > size_) {
		auto new_buffer = makeBuffer<Type>(size);
		if (getSize() > 0) {
			MemoryCopy(new_buffer.get(), buffer_.get(), sizeof(Type) * getSize());
		}
		buffer_ = std::move(new_buffer);
		size_ = size;
	}
}

template <typename Type, typename U>
void AudioBuffer<Type, U>::clear() {
	head_ = 0;
	tail_ = 0;
}

template <typename Type, typename U>
void AudioBuffer<Type, U>::fill(Type value) {
	MemorySet(buffer_.get(), value, sizeof(Type) * size_);
}

template <typename Type, typename U>
size_t AudioBuffer<Type, U>::getAvailableWrite() const {
	return getAvailableWrite(head_, tail_);
}

template <typename Type, typename U>
size_t AudioBuffer<Type, U>::getAvailableRead() const {
	return getAvailableRead(head_, tail_);
}

template <typename Type, typename U>
XAMP_ALWAYS_INLINE size_t AudioBuffer<Type, U>::getAvailableWrite(size_t head, size_t tail) const {
	auto result = tail - head - 1;
	if (head >= tail) {
		result += size_;
	}
	return result;
}

template <typename Type, typename U>
XAMP_ALWAYS_INLINE size_t AudioBuffer<Type, U>::getAvailableRead(size_t head, size_t tail) const {
	if (head >= tail) {
		return head - tail;
	}
	return head + size_ - tail;
}

template <typename Type, typename U>
bool AudioBuffer<Type, U>::tryWrite(const Type* data, size_t count) {
	const auto head = head_.load(std::memory_order_relaxed);
	const auto tail = tail_.load(std::memory_order_acquire);

	auto next_head = head + count;

	if (count > getAvailableWrite(head, tail)) {
		return false;
	}

	if (next_head > size_) {
		const auto range1 = size_ - head;
		const auto range2 = count - range1;
		MemoryCopy(buffer_.get() + head, data, range1 * sizeof(Type));
		MemoryCopy(buffer_.get(), data + range1, range2 * sizeof(Type));
		next_head -= size_;
	}
	else {
		MemoryCopy(buffer_.get() + head, data, count * sizeof(Type));
		if (next_head == size_) {
			next_head = 0;
		}
	}
	head_.store(next_head, std::memory_order_release);
	return true;
}

template <typename Type, typename U>
bool AudioBuffer<Type, U>::tryRead(Type* data, size_t count, size_t& num_filled_count) {
	const auto head = head_.load(std::memory_order_acquire);
	const auto tail = tail_.load(std::memory_order_relaxed);

	count = (std::min)(count, getAvailableRead(head, tail));
	num_filled_count = count;
	if (!count) {
		return false;
	}

	auto next_tail = tail + count;

	if (next_tail > size_) {
		const auto range1 = size_ - tail;
		const auto range2 = count - range1;
		MemoryCopy(data, buffer_.get() + tail, range1 * sizeof(Type));
		MemoryCopy(data + range1, buffer_.get(), range2 * sizeof(Type));
		next_tail -= size_;
	}
	else {
		MemoryCopy(data, buffer_.get() + tail, count * sizeof(Type));
		if (next_tail == size_) {
			next_tail = 0;
		}
	}
	tail_.store(next_tail, std::memory_order_release);
	return true;
}

XAMP_BASE_NAMESPACE_END