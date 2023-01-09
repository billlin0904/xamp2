//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <thread>
#include <base/stop_token.h>

namespace xamp::base {

#ifdef __cpp_lib_jthread

using JThread = std::jthread;

#else

class JThread {
public:
	using id = ::std::thread::id;
	using native_handle_type = ::std::thread::native_handle_type;

	JThread() noexcept;

	template
	<
		typename Callable,
		typename... Args,
		typename = ::std::enable_if_t<!::std::is_same_v<::std::decay_t<Callable>, JThread>>
	>
		explicit JThread(Callable&& cb, Args&&... args)
		: stop_source_{ kNoStopState }
		, thread_{ [](auto stop_token, auto&& cb, auto&&... args) {
        	if constexpr (std::is_invocable_v<Callable, StopToken, Args...>) {
        		std::invoke(
        			std::forward<decltype(cb)>(cb),
        			std::move(stop_token),
        			std::forward<decltype(args)>(args)...
        		);
        	}
			else {
				std::invoke(
        			std::forward<decltype(cb)>(cb),
        			std::forward<decltype(args)>(args)...
        		);
        	}
		},
		stop_source_.get_token(),
		std::forward<Callable>(cb),
		std::forward<Args>(args)... } {
	}

	~JThread();

	JThread(const JThread&) = delete;
	JThread(JThread&&) noexcept = default;
	JThread& operator=(const JThread&) = delete;
	JThread& operator=(JThread&&) noexcept;

	void swap(JThread&) noexcept;
	bool joinable() const noexcept;
	void join();
	void detach();

	id get_id() const noexcept;
	native_handle_type native_handle();

	static unsigned hardware_concurrency() noexcept {
		return std::thread::hardware_concurrency();
	}

	[[nodiscard]] StopSource get_stop_source() noexcept;
	[[nodiscard]] StopToken get_stop_token() const noexcept;

	bool request_stop() noexcept {
		return get_stop_source().request_stop();
	}
private:
	StopSource stop_source_;
	std::thread thread_;
};

inline JThread::JThread() noexcept
	: stop_source_{ kNoStopState } {
}

inline JThread::~JThread() {
	if (joinable()) {
		request_stop();
		join();
	}
}

inline JThread& JThread::operator=(JThread&& t) noexcept {
	if (joinable()) {
		request_stop();
		join();
	}

	stop_source_ = std::move(t.stop_source_);
	thread_ = std::move(t.thread_);
	return *this;
}

inline bool JThread::joinable() const noexcept {
	return thread_.joinable();
}

inline void JThread::join() {
	thread_.join();
}
inline void JThread::detach() {
	thread_.detach();
}

inline JThread::id JThread::get_id() const noexcept {
	return thread_.get_id();
}

inline typename JThread::native_handle_type JThread::native_handle() {
	return thread_.native_handle();
}

inline StopSource JThread::get_stop_source() noexcept {
	return stop_source_;
}

inline StopToken JThread::get_stop_token() const noexcept {
	return stop_source_.get_token();
}

inline void JThread::swap(JThread& t) noexcept {
	std::swap(stop_source_, t.stop_source_);
	std::swap(thread_, t.thread_);
}
#endif

}

