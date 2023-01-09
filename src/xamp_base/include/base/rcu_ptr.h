//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>

#include <base/base.h>

namespace xamp::base {

#ifdef __cpp_lib_atomic_shared_ptr

template <typename T>
using atomic_shared_ptr = std::atomic<std::shared_ptr<T>>;

#else

template <typename T>
class AtomicSharedPtr {
public:
	constexpr AtomicSharedPtr() noexcept = default;

	constexpr AtomicSharedPtr(std::shared_ptr<T> desired) noexcept
		: ptr_(desired) {
	}

	AtomicSharedPtr(const AtomicSharedPtr&) = delete;
	AtomicSharedPtr& operator= (const AtomicSharedPtr&) = delete;

	void operator= (std::shared_ptr<T> desired) noexcept {
		store(desired);
	}

	bool is_lock_free() const noexcept {
		return std::atomic_is_lock_free(&ptr_);
	}

	void store(std::shared_ptr<T> desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
		std::atomic_store_explicit(&ptr_, desired, order);
	}

	std::shared_ptr<T> load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
		return std::atomic_load_explicit(&ptr_, order);
	}

	operator std::shared_ptr<T>() const noexcept {
		return load();
	}

	std::shared_ptr<T> exchange(std::shared_ptr<T> desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
		return std::atomic_exchange_explicit(&ptr_, desired, order);
	}

	bool compare_exchange_weak(std::shared_ptr<T>& expected, const std::shared_ptr<T>& desired,
		std::memory_order success, std::memory_order failure) noexcept {
		return std::atomic_compare_exchange_weak_explicit(&ptr_, &expected, desired, success, failure);
	}

	bool compare_exchange_weak(std::shared_ptr<T>& expected, std::shared_ptr<T>&& desired,
		std::memory_order success, std::memory_order failure) noexcept {
		return std::atomic_compare_exchange_weak_explicit(&ptr_, &expected, desired, success, failure);
	}

	bool compare_exchange_weak(std::shared_ptr<T>& expected, const std::shared_ptr<T>& desired,
		std::memory_order order = std::memory_order_seq_cst) noexcept {
		return compare_exchange_weak(expected, desired, order, order);
	}

	bool compare_exchange_weak(std::shared_ptr<T>& expected, std::shared_ptr<T>&& desired,
		std::memory_order order = std::memory_order_seq_cst) noexcept {
		return compare_exchange_weak(expected, std::move(desired), order, order);
	}

	bool compare_exchange_strong(std::shared_ptr<T>& expected, const std::shared_ptr<T>& desired,
		std::memory_order success, std::memory_order failure) noexcept {
		return std::atomic_compare_exchange_strong_explicit(&ptr_, &expected, desired, success, failure);
	}

	bool compare_exchange_strong(std::shared_ptr<T>& expected, std::shared_ptr<T>&& desired,
		std::memory_order success, std::memory_order failure) noexcept {
		return std::atomic_compare_exchange_strong_explicit(&ptr_, &expected, desired, success, failure);
	}

	bool compare_exchange_strong(std::shared_ptr<T>& expected, const std::shared_ptr<T>& desired,
		std::memory_order order = std::memory_order_seq_cst) noexcept {
		return compare_exchange_strong(expected, desired, order, order);
	}

	bool compare_exchange_strong(std::shared_ptr<T>& expected, std::shared_ptr<T>&& desired,
		std::memory_order order = std::memory_order_seq_cst) noexcept {
		return compare_exchange_strong(expected, std::move(desired), order, order);
	}
private:
	std::shared_ptr<T> ptr_;
};

template <typename T>
using atomic_shared_ptr = AtomicSharedPtr<T>;
#endif

template
<
	template <typename>
	typename TAtomicSharedPtr
>
struct atomic_shared_ptr_traits {
	template <typename T>
	using atomic_shared_ptr = TAtomicSharedPtr<T>;

	template <typename T>
	using shared_ptr = std::shared_ptr<T>;

	template <typename T, typename... Args>
	static auto make_shared(Args &&...args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
};

template
<
	typename T,
	template <typename> class TAtomicSharedPtr = atomic_shared_ptr,
	typename Traits = atomic_shared_ptr_traits<TAtomicSharedPtr>
>
class RcuPtr {
public:
	template <typename S>
	using atomic_shared_ptr = typename Traits::template atomic_shared_ptr<S>;

	template <typename W>
	using shared_ptr = typename Traits::template shared_ptr<W>;

	using element_type = typename shared_ptr<const T>::element_type;
public:
	RcuPtr() = default;

	explicit RcuPtr(const shared_ptr<const T>& desired)
		: ptr_(desired) {
	}

	explicit RcuPtr(shared_ptr<const T>&& desired)
		: ptr_(std::move(desired)) {
	}

	~RcuPtr() = default;

	RcuPtr(const RcuPtr&) = delete;
	RcuPtr& operator=(const RcuPtr&) = delete;
	RcuPtr(RcuPtr&&) = delete;
	RcuPtr& operator=(RcuPtr&&) = delete;

	shared_ptr<const T> read() const noexcept {
		return ptr_.load(std::memory_order_consume);
	}

	void reset(const shared_ptr<T>& r) noexcept {
		ptr_.store(r, std::memory_order_release);
	}

	void reset(shared_ptr<T>&& r) noexcept {
		ptr_.store(std::move(r), std::memory_order_release);
	}

	template <typename F>
	void copy_update(F&& fun) {
		auto sp_ptr_copy = ptr_.load(std::memory_order_consume);
		shared_ptr<T> r;
		do {
			if (sp_ptr_copy) {
				r = Traits::template make_shared<T>(*sp_ptr_copy);
			}

			std::forward<F>(fun)(r.get());
		} while (!ptr_.compare_exchange_strong(sp_ptr_copy, std::move(r),
			std::memory_order_release,
			std::memory_order_consume));
	}
private:
	atomic_shared_ptr<const T> ptr_;
};

}
