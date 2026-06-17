//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#include <utility>

XAMP_BASE_NAMESPACE_BEGIN

template <typename t, typename Traits>
class UniqueHandle final {
public:
	explicit UniqueHandle(t value = Traits::invalid()) : value_(value) {
	}

	UniqueHandle(UniqueHandle&& other) : value_(other.release()) {
	}

	UniqueHandle& operator=(UniqueHandle&& other) {
		reset(other.release());
		return *this;
	}

	~UniqueHandle() {
		close();
	}

	XAMP_DISABLE_COPY(UniqueHandle)

	[[nodiscard]] t get() const {
		return value_;
	}

	void reset(t value = Traits::invalid()) {
		if (value_ != value) {
			close();
			value_ = value;
		}
	}

	[[nodiscard]] t release() {
		auto value = value_;
		value_ = Traits::invalid();
		return value;
	}

	[[nodiscard]] bool is_valid() const {
		return value_ != Traits::invalid();
	}

	void swap(UniqueHandle& other) noexcept {
		std::swap(value_, other.value_);
	}

	explicit operator bool() const {
		return is_valid();
	}

	void close()  {
		if (is_valid()) {
			Traits::close(value_);
			value_ = Traits::invalid();
		}
	}
private:
	bool operator==(UniqueHandle const &);
	bool operator!=(UniqueHandle const &);	

	t value_;
};

template <typename t, typename Traits>
auto swap(UniqueHandle<t, Traits> & left, UniqueHandle<t, Traits> & right) -> void {
	left.swap(right);
}

template <typename t, typename Traits>
auto operator==(UniqueHandle<t, Traits> const & left, UniqueHandle<t, Traits> const & right) -> bool {
	return left.get() == right.get();
}

template <typename t, typename Traits>
auto operator!=(UniqueHandle<t, Traits> const & left, UniqueHandle<t, Traits> const & right) -> bool {
	return left.get() != right.get();
}

template <typename t, typename Traits>
auto operator<(UniqueHandle<t, Traits> const & left, UniqueHandle<t, Traits> const & right) -> bool {
	return left.get() < right.get();
}

template <typename t, typename Traits>
auto operator>=(UniqueHandle<t, Traits> const & left, UniqueHandle<t, Traits> const & right) -> bool {
	return left.get() >= right.get();
}

template <typename t, typename Traits>
auto operator>(UniqueHandle<t, Traits> const & left, UniqueHandle<t, Traits> const & right) -> bool {
	return left.get() > right.get();
}

template <typename t, typename Traits>
auto operator<=(UniqueHandle<t, Traits> const & left, UniqueHandle<t, Traits> const & right) -> bool {
	return left.get() <= right.get();
}

XAMP_BASE_NAMESPACE_END
