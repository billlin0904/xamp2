//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
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

namespace internal_lock_free {
	template <typename T, T TValue>
	T dequeue(std::atomic<T>& queue) noexcept {
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
	T dequeue_wait(std::atomic<uint8_t>& state, T& queue) noexcept {
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
	void enqueue(T element, std::atomic<T>& queue) noexcept {
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
	void enqueue_wait(U&& element, std::atomic<uint8_t>& state, T& queue) noexcept {
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

template 
<
	typename Type,
	typename U =
	std::enable_if_t 
	<
		std::is_nothrow_move_assignable_v<Type>
	>
>
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
		while (!is_empty()) {
			dequeue();
		}
	}

	template <typename T>
	bool try_enqueue(T&& element) noexcept {
		auto head = head_.load(std::memory_order_relaxed);

		do {
			if (head - tail_.load(std::memory_order_relaxed) >= capacity_)
				return false;

			XAMP_LIKELY(head_.compare_exchange_weak(head, head + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
				break;
			}
		} while (true);
		
		internal_enqueue(std::forward<T>(element), head);
		return true;
	}

	template <typename T>
	bool try_dequeue(T& element) noexcept {
		auto tail = tail_.load(std::memory_order_relaxed);

		do {
			if (head_.load(std::memory_order_relaxed) - tail <= 0)
				return false;

			XAMP_LIKELY(tail_.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
				break;
			}
		} while (true); // This loop is not FIFO.

		element = internal_dequeue(tail);
		return true;
	}

	template <typename T>
	void enqueue(T&& element) noexcept {
		//const auto head = head_.fetch_add(1, std::memory_order_seq_cst);
		const auto head = head_.fetch_add(1, std::memory_order_relaxed);
		internal_enqueue(element, head);
	}

	Type dequeue() noexcept {
		//const auto tail = tail_.fetch_add(1, std::memory_order_seq_cst);
		const auto tail = tail_.fetch_add(1, std::memory_order_relaxed);
		return internal_dequeue(tail);
	}

	XAMP_NO_DISCARD bool is_empty() const noexcept {
		return size() == 0;
	}

	XAMP_NO_DISCARD size_t size() const noexcept {
		ptrdiff_t diff = head_.load(std::memory_order_acquire) -
			tail_.load(std::memory_order_acquire);
		if (diff < 0) {
			diff += capacity_;
		}
		return static_cast<size_t>(diff);
	}

private:
    template <typename E>
    void internal_enqueue(E&& element, size_t head) noexcept {
		const auto index = head % capacity_;
        internal_lock_free::enqueue_wait(std::forward<E>(element), states_[index], queue_[index]);
	}

	Type internal_dequeue(size_t tail) noexcept {
		const auto index = tail % capacity_;
		auto temp = internal_lock_free::dequeue_wait(states_[index], queue_[index]);

		if constexpr (!std::is_trivially_destructible_v<Type>) {
			queue_[index].~Type();
		}		
		return temp;
	}

	XAMP_CACHE_ALIGNED(kCacheAlignSize) size_t capacity_;
	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<size_t> head_;
	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<size_t> tail_;
	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<uint8_t>* states_;
	XAMP_CACHE_ALIGNED(kCacheAlignSize) Type* queue_;
	XAMP_CACHE_ALIGNED(kCacheAlignSize) QueueAllocator queue_allocator_;
	XAMP_CACHE_ALIGNED(kCacheAlignSize) StateAllocator state_allocator_;
};


XAMP_BASE_NAMESPACE_END

