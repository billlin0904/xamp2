//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

template <typename T, typename Traits>
class XAMP_BASE_API_ONLY_EXPORT UniqueHandle final {
public:
	explicit UniqueHandle(T value = Traits::invalid()) : value_(value) {
	}

	UniqueHandle(UniqueHandle&& other) : value_(other.release()) {
	}

	UniqueHandle& operator=(UniqueHandle&& other) {
		reset(other.release());
		return *this;
	}

	~UniqueHandle() {
		Close();
	}

	XAMP_DISABLE_COPY(UniqueHandle)

	[[nodiscard]] T get() const {
		return value_;
	}

	void reset(T value = Traits::invalid()) {
		if (value_ != value) {
			Close();
			value_ = value;
		}
	}

	[[nodiscard]] T release() {
		auto value = value_;
		value_ = Traits::invalid();
		return value;
	}

	[[nodiscard]] bool is_valid() const {
		return value_ != Traits::invalid();
	}

	explicit operator bool() const {
		return is_valid();
	}

	void Close()  {
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
auto swap(UniqueHandle<T, Traits> & left, UniqueHandle<T, Traits> & right) -> void {
	left.swap(right);
}

template <typename T, typename Traits>
auto operator==(UniqueHandle<T, Traits> const & left, UniqueHandle<T, Traits> const & right) -> bool {
	return left.get() == right.get();
}

template <typename T, typename Traits>
auto operator!=(UniqueHandle<T, Traits> const & left, UniqueHandle<T, Traits> const & right) -> bool {
	return left.get() != right.get();
}

template <typename T, typename Traits>
auto operator<(UniqueHandle<T, Traits> const & left, UniqueHandle<T, Traits> const & right) -> bool {
	return left.get() < right.get();
}

template <typename T, typename Traits>
auto operator>=(UniqueHandle<T, Traits> const & left, UniqueHandle<T, Traits> const & right) -> bool {
	return left.get() >= right.get();
}

template <typename T, typename Traits>
auto operator>(UniqueHandle<T, Traits> const & left, UniqueHandle<T, Traits> const & right) -> bool {
	return left.get() > right.get();
}

template <typename T, typename Traits>
auto operator<=(UniqueHandle<T, Traits> const & left, UniqueHandle<T, Traits> const & right) -> bool {
	return left.get() <= right.get();
}

XAMP_BASE_NAMESPACE_END
