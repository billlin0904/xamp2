//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

template <typename T, typename Traits>
class XAMP_BASE_API_ONLY_EXPORT UniqueHandle final {
public:
	explicit UniqueHandle(T value = Traits::invalid()) noexcept 
		: value_(value) {
	}

	UniqueHandle(UniqueHandle&& other) noexcept
		: value_(other.release()) {
	}

	UniqueHandle& operator=(UniqueHandle&& other) noexcept {
		reset(other.release());
		return *this;
	}

	~UniqueHandle() noexcept {
		close();
	}

	XAMP_DISABLE_COPY(UniqueHandle)

	T get() const noexcept {
		return value_;
	}

	void reset(T value = Traits::invalid()) noexcept {
		if (value_ != value) {
			close();
			value_ = value;
		}
	}

	XAMP_NO_DISCARD T release() noexcept {
		auto value = value_;
		value_ = Traits::invalid();
		return value;
	}

	XAMP_NO_DISCARD bool is_valid() const noexcept {
		return value_ != Traits::invalid();
	}

	explicit operator bool() const noexcept {
		return is_valid();
	}

	void close() noexcept(noexcept(Traits::close(value_))) {
		if (is_valid()) {
			Traits::close(value_);
			value_ = Traits::invalid();
		}
	}
private:
	bool operator==(UniqueHandle const &);
	bool operator!=(UniqueHandle const &);	

	T value_;
};

template <typename T, typename Traits>
auto swap(UniqueHandle<T, Traits> & left, UniqueHandle<T, Traits> & right) noexcept -> void {
	left.swap(right);
}

template <typename T, typename Traits>
auto operator==(UniqueHandle<T, Traits> const & left, UniqueHandle<T, Traits> const & right) noexcept -> bool {
	return left.get() == right.get();
}

template <typename T, typename Traits>
auto operator!=(UniqueHandle<T, Traits> const & left, UniqueHandle<T, Traits> const & right) noexcept -> bool {
	return left.get() != right.get();
}

template <typename T, typename Traits>
auto operator<(UniqueHandle<T, Traits> const & left, UniqueHandle<T, Traits> const & right) noexcept -> bool {
	return left.get() < right.get();
}

template <typename T, typename Traits>
auto operator>=(UniqueHandle<T, Traits> const & left, UniqueHandle<T, Traits> const & right) noexcept -> bool {
	return left.get() >= right.get();
}

template <typename T, typename Traits>
auto operator>(UniqueHandle<T, Traits> const & left, UniqueHandle<T, Traits> const & right) noexcept -> bool {
	return left.get() > right.get();
}

template <typename T, typename Traits>
auto operator<=(UniqueHandle<T, Traits> const & left, UniqueHandle<T, Traits> const & right) noexcept -> bool {
	return left.get() <= right.get();
}

XAMP_BASE_NAMESPACE_END
