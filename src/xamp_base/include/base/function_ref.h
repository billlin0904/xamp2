//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <type_traits>
#include <functional>

#include <base/base.h>

namespace xamp::base {

template <typename FunctionType>
class FunctionRef; 

template <typename ReturnType, typename... Args>
class XAMP_BASE_API_ONLY_EXPORT FunctionRef<ReturnType(Args...)> final {
public:
	constexpr FunctionRef() noexcept
		: callable_(0)
		, callback_(&FunctionRef::InvalidCall) {
	}

	constexpr FunctionRef(const FunctionRef& other) noexcept = default;
	constexpr FunctionRef& operator=(const FunctionRef& other) noexcept = default;

	template <typename Fun,
		typename std::enable_if_t<
		std::is_invocable_r_v<ReturnType, Fun, Args...>, int> = 0>
	constexpr FunctionRef(Fun && fun) noexcept {
		callable_ = reinterpret_cast<intptr_t>(&fun);
		callback_ = &FunctionRef::MakeCallback<typename std::remove_reference<Fun>::type>;
	}

	constexpr FunctionRef& operator=(std::nullptr_t) noexcept {		
		callback_ = reinterpret_cast<intptr_t>(&FunctionRef::InvalidCall);
		callable_ = 0;
		return *this;
	}

	constexpr friend bool operator!=(FunctionRef<ReturnType(Args...)> ref, std::nullptr_t)  noexcept {
		return ref.callable_ != 0;
	}

	constexpr friend bool operator==(FunctionRef<ReturnType(Args...)> ref, std::nullptr_t)  noexcept {
		return ref.callable_ == 0;
	}

	constexpr friend bool operator==(const FunctionRef& ref,  const FunctionRef& other) noexcept {
		return (ref.callable_ == other.callable_);
	}

	constexpr void swap(FunctionRef& other) noexcept {
		std::swap(callable_, other.callable_);
		std::swap(callback_, other.callback_);
	}

	constexpr explicit operator bool() const noexcept { 
		return callable_;
	}	

	ReturnType operator()(Args ... args) const {
		return callback_(callable_, std::forward<Args>(args)...);
	}
private:
	template <typename Callable>
	static constexpr ReturnType MakeCallback(intptr_t callable, Args ... args) noexcept {
		return (*reinterpret_cast<Callable*>(callable))(std::forward<Args>(args)...);
	}

	using Callable = ReturnType(*)(intptr_t, Args ...);

	static ReturnType InvalidCall(intptr_t, Args ...) {
		throw std::bad_function_call();
	}

	Callable callback_;
	intptr_t callable_;
};

}
