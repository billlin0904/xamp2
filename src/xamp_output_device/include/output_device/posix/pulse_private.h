//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>

#include <pulse/pulseaudio.h>

#include <output_device/output_device.h>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

struct PulseMainloopDeleter {
	void operator()(pa_mainloop* mainloop) const noexcept {
		if (mainloop != nullptr) {
			::pa_mainloop_free(mainloop);
		}
	}
};

struct PulseThreadedMainloopDeleter {
	void operator()(pa_threaded_mainloop* mainloop) const noexcept {
		if (mainloop != nullptr) {
			::pa_threaded_mainloop_free(mainloop);
		}
	}
};

struct PulseContextDeleter {
	void operator()(pa_context* context) const noexcept {
		if (context != nullptr) {
			::pa_context_disconnect(context);
			::pa_context_unref(context);
		}
	}
};

struct PulseStreamDeleter {
	void operator()(pa_stream* stream) const noexcept {
		if (stream != nullptr) {
			::pa_stream_disconnect(stream);
			::pa_stream_unref(stream);
		}
	}
};

struct PulseOperationDeleter {
	void operator()(pa_operation* operation) const noexcept {
		if (operation != nullptr) {
			::pa_operation_unref(operation);
		}
	}
};

using PulseMainloopPtr = std::unique_ptr<pa_mainloop, PulseMainloopDeleter>;
using PulseThreadedMainloopPtr = std::unique_ptr<pa_threaded_mainloop, PulseThreadedMainloopDeleter>;
using PulseContextPtr = std::unique_ptr<pa_context, PulseContextDeleter>;
using PulseStreamPtr = std::unique_ptr<pa_stream, PulseStreamDeleter>;
using PulseOperationPtr = std::unique_ptr<pa_operation, PulseOperationDeleter>;

class PulseThreadedMainloopLock final {
public:
	explicit PulseThreadedMainloopLock(pa_threaded_mainloop* mainloop) noexcept
		: mainloop_(mainloop) {
		if (mainloop_ != nullptr) {
			::pa_threaded_mainloop_lock(mainloop_);
		}
	}

	~PulseThreadedMainloopLock() {
		if (mainloop_ != nullptr) {
			::pa_threaded_mainloop_unlock(mainloop_);
		}
	}

	XAMP_DISABLE_COPY(PulseThreadedMainloopLock)

private:
	pa_threaded_mainloop* mainloop_{ nullptr };
};

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
