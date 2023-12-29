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

enum MpmcState : uint8_t {
	MPMC_Q_EMPTY,
	MPMC_Q_STORING,
	MPMC_Q_STORED,
	MPMC_Q_LOADING
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
			uint8_t expected = MPMC_Q_STORED;

			XAMP_LIKELY(state.compare_exchange_weak(expected, MPMC_Q_LOADING, std::memory_order_acquire, std::memory_order_relaxed)) {
				T element{ std::move(queue) };
				state.store(MPMC_Q_EMPTY, std::memory_order_release);
				return element;
			}

			// Do speculative loads while busy-waiting to avoid broadcasting RFO messages.
			do
				CpuRelax();
			while (state.load(std::memory_order_relaxed) != MPMC_Q_STORED);
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
			uint8_t expected = MPMC_Q_EMPTY;

			XAMP_LIKELY(state.compare_exchange_weak(expected, MPMC_Q_STORING, std::memory_order_acquire, std::memory_order_relaxed)) {
				queue = std::forward<U>(element);
				state.store(MPMC_Q_STORED, std::memory_order_release);
				return;
			}

			// Do speculative loads while busy-waiting to avoid broadcasting RFO messages.
			do
				CpuRelax();
			while (state.load(std::memory_order_relaxed) != MPMC_Q_EMPTY);
		}
	}
}

template <typename Type>
class XAMP_BASE_API_ONLY_EXPORT MpmcQueue {
public:
	using QueueAllocator = std::allocator<Type>;
	using StateAllocator = std::allocator<std::atomic<uint8_t>>;

	using QueueAllocatorTraits = std::allocator_traits<QueueAllocator>;
	using StateAllocatorTraits = std::allocator_traits<StateAllocator>;

	explicit MpmcQueue(size_t capacity)
		: capacity_(NextPowerOfTwo(capacity))
		, head_(0)
		, tail_(0) {
		XAMP_EXPECTS(capacity > 0);
		states_ = StateAllocatorTraits::allocate(state_allocator_, capacity_);
		queue_ = QueueAllocatorTraits::allocate(queue_allocator_, capacity_);
		for (size_t i = 0; i < capacity_; ++i) {
			QueueAllocatorTraits::construct(queue_allocator_, &queue_[i]);
		}
		for (size_t i = 0; i < capacity_; ++i) {
			states_[i].store(MpmcState::MPMC_Q_EMPTY);
		}
	}

	~MpmcQueue() {
		clear();
		StateAllocatorTraits::deallocate(state_allocator_, states_, capacity_);
		QueueAllocatorTraits::deallocate(queue_allocator_, queue_, capacity_);
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

	[[nodiscard]] bool empty() const noexcept {
		return size() == 0;
	}

	[[nodiscard]] size_t size() const noexcept {
		ptrdiff_t diff = head_.load(std::memory_order_acquire) -
			tail_.load(std::memory_order_acquire);
		if (diff < 0) {
			diff += capacity_;
		}
		return static_cast<size_t>(diff);
	}

private:
	template <typename U>
	void DoEnqueue(U&& element, size_t head) noexcept {
		const auto index = head % capacity_;
		LockFree::EnqueueWait(std::forward<U>(element), states_[index], queue_[index]);
	}

	Type DoDequeue(size_t tail) noexcept {
		const auto index = tail % capacity_;
		auto temp = LockFree::DequeueWait(states_[index], queue_[index]);
		queue_[index].~Type();
		return temp;
	}

	XAMP_CACHE_ALIGNED(kCacheAlignSize) size_t capacity_;
	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<size_t> head_;
	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<size_t> tail_;
	QueueAllocator queue_allocator_;
	StateAllocator state_allocator_;
	std::atomic<uint8_t>* states_;
	Type* queue_;
};


XAMP_BASE_NAMESPACE_END

