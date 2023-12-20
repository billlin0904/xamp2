//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/assert.h>
#include <base/platform.h>
#include <base/math.h>

XAMP_BASE_NAMESPACE_BEGIN

enum State : uint8_t {
	EMPTY,
	STORING,
	STORED,
	LOADING
};

namespace LockFree {
	template <typename T, T TValue>
	T Dequeue(std::atomic<T>& queue) noexcept {
		for (;;) {
			auto element = queue.exchange(TValue, std::memory_order_acquire);

			XAMP_LIKELY(element != TValue) {
				return element;
			}

			do
				CpuRelax();
			while (queue.load(std::memory_order_relaxed) == TValue);
		}
	}

	template <typename T>
	T DequeueWait(std::atomic<uint8_t>& state, T& queue) noexcept {
		for (;;) {
			uint8_t expected = STORED;

			XAMP_LIKELY(state.compare_exchange_weak(expected, LOADING, std::memory_order_acquire, std::memory_order_relaxed)) {
				T element{ std::move(queue) };
				state.store(EMPTY, std::memory_order_release);
				return element;
			}

			// Do speculative loads while busy-waiting to avoid broadcasting RFO messages.
			do
				CpuRelax();
			while (state.load(std::memory_order_relaxed) != STORED);
		}
	}

	template <typename T, T TValue>
	void Enqueue(T element, std::atomic<T>& queue) noexcept {
		for (T expected = TValue;; expected = TValue) {
			XAMP_LIKELY(queue.compare_exchange_weak(expected, element, std::memory_order_release, std::memory_order_relaxed)) {
				break;
			}

			do
				CpuRelax();
			while (queue.load(std::memory_order_relaxed) != TValue);
		}
	}

	template <typename U, typename T>
	void EnqueueWait(U&& element, std::atomic<uint8_t>& state, T& queue) noexcept {
		for (;;) {
			uint8_t expected = EMPTY;

			XAMP_LIKELY(state.compare_exchange_weak(expected, STORING, std::memory_order_acquire, std::memory_order_relaxed)) {
				queue = std::forward<U>(element);
				state.store(STORED, std::memory_order_release);
				return;
			}

			// Do speculative loads while busy-waiting to avoid broadcasting RFO messages.
			do
				CpuRelax();
			while (state.load(std::memory_order_relaxed) != EMPTY);
		}
	}
}

template <typename Type>
class XAMP_BASE_API_ONLY_EXPORT MpmcQueue {
public:
	explicit MpmcQueue(size_t capacity)
		: capacity_(capacity)
		, head_(0)
		, tail_(0) {
		states_ = std::allocator_traits<std::allocator<std::atomic<uint8_t>>>::allocate(state_allocator_, capacity_);
		queue_ = std::allocator_traits<std::allocator<Type>>::allocate(allocator_, capacity_);
		for (size_t i = 0; i < capacity_; ++i) {
			new (&queue_[i]) Type();
		}
		for (size_t i = 0; i < capacity_; ++i) {
			states_[i].store(State::EMPTY);
		}
	}

	~MpmcQueue() {
		clear();
		std::allocator_traits<std::allocator<std::atomic<uint8_t>>>::deallocate(state_allocator_,
			states_,
			capacity_);
		std::allocator_traits<std::allocator<Type>>::deallocate(allocator_,
			queue_,
			capacity_);
	}

	void clear() {
		while (!empty()) {
			Dequeue();
		}
	}

	template <typename T>
	bool TryEnqueue(T&& element) noexcept {
		auto head = head_.load(std::memory_order_relaxed);

		do {
			if (head - tail_.load(std::memory_order_relaxed) >= capacity_)
				return false;

			XAMP_LIKELY(head_.compare_exchange_weak(head, head + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
				break;
			}
		} while (true);
		
		DoEnqueue(std::forward<T>(element), head);
		return true;
	}

	std::optional<Type> TryDequeue() noexcept {
		Type element;
		if (!TryDequeue(element)) {
			return std::nullopt;
		}
		return element;
	}

	template <typename T>
	bool TryDequeue(T& element) noexcept {
		auto tail = tail_.load(std::memory_order_relaxed);

		do {
			if (head_.load(std::memory_order_relaxed) - tail <= 0)
				return false;

			XAMP_LIKELY(tail_.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
				break;
			}
		} while (true); // This loop is not FIFO.

		element = DoDequeue(tail);
		return true;
	}

	template <typename T>
	void Enqueue(T&& element) noexcept {
		const auto head = head_.fetch_add(1, std::memory_order_seq_cst);
		DoEnqueue(element, head);
	}

	Type Dequeue() noexcept {
		const auto tail = tail_.fetch_add(1, std::memory_order_seq_cst);
		return DoDequeue(tail);
	}

	bool empty() const noexcept {
		return size() == 0;
	}

	size_t size() const noexcept {
		std::ptrdiff_t diff = head_.load(std::memory_order_acquire) -
			tail_.load(std::memory_order_acquire);
		if (diff < 0) {
			diff += capacity_;
		}
		return static_cast<size_t>(diff);
	}

private:
	template <typename U>
	void DoEnqueue(U&& element, size_t head) {
		const auto index = head % capacity_;
		LockFree::EnqueueWait(std::forward<U>(element), states_[index], queue_[index]);
	}

	Type DoDequeue(size_t tail) {
		const auto index = tail % capacity_;
		auto temp = LockFree::DequeueWait(states_[index], queue_[index]);
		queue_[index].~Type();
		return temp;
	}

	XAMP_CACHE_ALIGNED(kCacheAlignSize) size_t capacity_;
	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<size_t> head_;
	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<size_t> tail_;
	std::allocator<Type> allocator_;
	std::allocator<std::atomic<uint8_t>> state_allocator_;
	std::atomic<uint8_t>* states_;
	Type* queue_;
	//Vector<Type> queue_;
};


XAMP_BASE_NAMESPACE_END

