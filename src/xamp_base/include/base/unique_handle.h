//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
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
		Close();
	}

	XAMP_DISABLE_COPY(UniqueHandle)

	[[nodiscard]] T get() const noexcept {
		return value_;
	}

	void reset(T value = Traits::invalid()) noexcept {
		if (value_ != value) {
			Close();
			value_ = value;
		}
	}

	[[nodiscard]] T release() noexcept {
		auto value = value_;
		value_ = Traits::invalid();
		return value;
	}

	[[nodiscard]] bool is_valid() const noexcept {
		return value_ != Traits::invalid();
	}

	explicit operator bool() const noexcept {
		return is_valid();
	}

	void Close() noexcept(noexcept(Traits::Close(value_))) {
		if (is_valid()) {
			Traits::Close(value_);
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
