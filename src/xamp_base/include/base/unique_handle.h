//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

namespace xamp::base {

template <typename T, typename Traits>
class XAMP_BASE_API_ONLY_EXPORT UniqueHandle final {
public:
	struct boolean_struct { int member; };
	typedef int boolean_struct::* boolean_type;

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

	~UniqueHandle() {
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

	T release() noexcept {
		auto value = value_;
		value_ = Traits::invalid();
		return value;
	}

	bool is_valid() const noexcept {
		return value_ != Traits::invalid();
	}

	operator boolean_type() const noexcept {
		return Traits::invalid() != value_ ? &boolean_struct::member : nullptr;
	}

	void close() noexcept {
		if (value_ != Traits::invalid()) {
			Traits::close(value_);
		}
	}
private:
	bool operator==(UniqueHandle const &);
	bool operator!=(UniqueHandle const &);	

	T value_;
};

template <typename T, typename Traits>
auto swap(UniqueHandle<T, Traits> & left, UniqueHandle<T, Traits> & right) throw() -> void {
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

}
