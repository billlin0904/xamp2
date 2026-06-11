//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>

#include <pipewire/pipewire.h>

#include <output_device/output_device.h>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

void EnsurePipeWireInitialized();

struct PipeWireThreadLoopDeleter {
	void operator()(pw_thread_loop* loop) const noexcept {
		if (loop != nullptr) {
			::pw_thread_loop_destroy(loop);
		}
	}
};

struct PipeWireContextDeleter {
	void operator()(pw_context* context) const noexcept {
		if (context != nullptr) {
			::pw_context_destroy(context);
		}
	}
};

struct PipeWireCoreDeleter {
	void operator()(pw_core* core) const noexcept {
		if (core != nullptr) {
			::pw_core_disconnect(core);
		}
	}
};

struct PipeWireRegistryDeleter {
	void operator()(pw_registry* registry) const noexcept {
		if (registry != nullptr) {
			::pw_proxy_destroy(reinterpret_cast<pw_proxy*>(registry));
		}
	}
};

struct PipeWireStreamDeleter {
	void operator()(pw_stream* stream) const noexcept {
		if (stream != nullptr) {
			::pw_stream_destroy(stream);
		}
	}
};

using PipeWireThreadLoopPtr = std::unique_ptr<pw_thread_loop, PipeWireThreadLoopDeleter>;
using PipeWireContextPtr = std::unique_ptr<pw_context, PipeWireContextDeleter>;
using PipeWireCorePtr = std::unique_ptr<pw_core, PipeWireCoreDeleter>;
using PipeWireRegistryPtr = std::unique_ptr<pw_registry, PipeWireRegistryDeleter>;
using PipeWireStreamPtr = std::unique_ptr<pw_stream, PipeWireStreamDeleter>;

class PipeWireThreadLoopLock final {
public:
	explicit PipeWireThreadLoopLock(pw_thread_loop* loop) noexcept
		: loop_(loop) {
		if (loop_ != nullptr) {
			::pw_thread_loop_lock(loop_);
		}
	}

	~PipeWireThreadLoopLock() {
		if (loop_ != nullptr) {
			::pw_thread_loop_unlock(loop_);
		}
	}

	XAMP_DISABLE_COPY(PipeWireThreadLoopLock)

private:
	pw_thread_loop* loop_{ nullptr };
};

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
