//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>

#include <base/base.h>
#include <base/align_ptr.h>
#include <base/memory.h>
#include <base/vmmemlock.h>

namespace xamp::base {

template 
<
	typename Type,
	typename U = std::enable_if_t<std::is_trivially_copyable<Type>::value>
>
class XAMP_BASE_API_ONLY_EXPORT AudioBuffer final {
public:
	AudioBuffer() noexcept;

	explicit AudioBuffer(size_t size);

	~AudioBuffer() noexcept;

	XAMP_DISABLE_COPY(AudioBuffer)

	Type* GetData() const noexcept;
	
	void Clear() noexcept;

	void Resize(size_t size);

	size_t GetSize() const noexcept;

    size_t GetAvailableWrite() const noexcept;

    size_t GetAvailableRead() const noexcept;

    bool TryWrite(const Type* data, size_t count) noexcept;

    bool TryRead(Type* data, size_t count) noexcept;

	void Fill(Type value) noexcept;

private:
    size_t GetAvailableWrite(size_t head, size_t tail) const noexcept;

    size_t GetAvailableRead(size_t head, size_t tail) const noexcept;

    XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<size_t> head_;
    XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<size_t> tail_;
	size_t size_;
    AlignBufferPtr<Type> buffer_;
	VmMemLock lock_;
};

template <typename Type, typename U>
AudioBuffer<Type, U>::AudioBuffer() noexcept
	: head_(0)
	, tail_(0)
	, size_(0) {
}

template <typename Type, typename U>
AudioBuffer<Type, U>::AudioBuffer(size_t size)
    : AudioBuffer() {
    Resize(size);
}

template <typename Type, typename U>
AudioBuffer<Type, U>::~AudioBuffer() = default;

template <typename Type, typename U>
Type* AudioBuffer<Type, U>::GetData() const noexcept {
	return buffer_.get();
}

template <typename Type, typename U>
size_t AudioBuffer<Type, U>::GetSize() const noexcept {
	return size_;
}

template <typename Type, typename U>
void AudioBuffer<Type, U>::Resize(size_t size) {
	if (size > size_) {
		lock_.UnLock();
        auto new_buffer = MakeBuffer<Type>(size);
		(void)FastMemcpy(new_buffer.get(), buffer_.get(), sizeof(Type) * GetSize());
        buffer_ = std::move(new_buffer);
		size_ = size;
		lock_.Lock(buffer_.get(), sizeof(Type) * size);
	}
}

template <typename Type, typename U>
void AudioBuffer<Type, U>::Clear() noexcept {
	head_ = 0;
	tail_ = 0;
}

template <typename Type, typename U>
void AudioBuffer<Type, U>::Fill(Type value) noexcept {
    (void)std::memset(buffer_.get(), value, sizeof(Type) * size_);
}

template <typename Type, typename U>
size_t AudioBuffer<Type, U>::GetAvailableWrite() const noexcept {
	return GetAvailableWrite(head_, tail_);
}

template <typename Type, typename U>
size_t AudioBuffer<Type, U>::GetAvailableRead() const noexcept {
	return GetAvailableRead(head_, tail_);
}

template <typename Type, typename U>
XAMP_ALWAYS_INLINE size_t AudioBuffer<Type, U>::GetAvailableWrite(size_t head, size_t tail) const noexcept {
	auto result = tail - head - 1;
	if (head >= tail) {
		result += size_;
	}
	return result;
}

template <typename Type, typename U>
XAMP_ALWAYS_INLINE size_t AudioBuffer<Type, U>::GetAvailableRead(size_t head, size_t tail) const noexcept {
	if (head >= tail) {
		return head - tail;
	}
	return head + size_ - tail;
}

template <typename Type, typename U>
bool AudioBuffer<Type, U>::TryWrite(const Type* data, size_t count) noexcept {
    const auto head = head_.load(std::memory_order_relaxed);
    const auto tail = tail_.load(std::memory_order_acquire);

    auto next_head = head + count;

	if (count > GetAvailableWrite(head, tail)) {
		return false;
	}	

	if (next_head > size_) {
        const auto range1 = size_ - head;
        const auto range2 = count - range1;
		(void)FastMemcpy(buffer_.get() + head, data, range1 * sizeof(Type));
		(void)FastMemcpy(buffer_.get(), data + range1, range2 * sizeof(Type));
		next_head -= size_;
	} else {
		(void)FastMemcpy(buffer_.get() + head, data, count * sizeof(Type));
		if (next_head == size_) {
			next_head = 0;
		}
	}
	head_.store(next_head, std::memory_order_release);
	return true;
}

template <typename Type, typename U>
bool AudioBuffer<Type, U>::TryRead(Type* data, size_t count) noexcept {
    const auto head = head_.load(std::memory_order_acquire);
    const auto tail = tail_.load(std::memory_order_relaxed);

    auto next_tail = tail + count;

	if (count > GetAvailableRead(head, tail)) {
		return false;
	}	

	if (next_tail > size_) {
        const auto range1 = size_ - tail;
        const auto range2 = count - range1;
        (void)FastMemcpy(data, buffer_.get() + tail, range1 * sizeof(Type));
		(void)FastMemcpy(data + range1, buffer_.get(), range2 * sizeof(Type));
		next_tail -= size_;
	}
	else {
		(void)FastMemcpy(data, buffer_.get() + tail, count * sizeof(Type));
		if (next_tail == size_) {
			next_tail = 0;
		}
	}
	tail_.store(next_tail, std::memory_order_release);
	return true;
}

}
