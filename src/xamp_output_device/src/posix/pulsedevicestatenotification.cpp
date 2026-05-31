//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <output_device/posix/pulsedevicestatenotification.h>
#include <base/exception.h>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

PulseDeviceStateNotification::PulseDeviceStateNotification(std::weak_ptr<IDeviceStateListener> callback)
	: callback_(std::move(callback))
	, logger_(XampLoggerFactory.GetLogger(XAMP_LOG_NAME(PulseDeviceStateNotification))) {
}

PulseDeviceStateNotification::~PulseDeviceStateNotification() {
	Stop();
}

void PulseDeviceStateNotification::Run() {
	if (is_running_.exchange(true)) {
		return;
	}

	try {
		mainloop_.reset(::pa_threaded_mainloop_new());
		if (!mainloop_) {
			Throw<PlatformException>("PulseAudio threaded mainloop create failed.");
		}

		auto* api = ::pa_threaded_mainloop_get_api(mainloop_.get());
		context_.reset(::pa_context_new(api, "XAMP device notification"));
		if (!context_) {
			Throw<PlatformException>("PulseAudio notification context create failed.");
		}

		::pa_context_set_state_callback(context_.get(), &PulseDeviceStateNotification::ContextStateCallback, this);
		if (::pa_context_connect(context_.get(), nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
			Throw<PlatformException>("PulseAudio notification connect failed: {}",
				::pa_strerror(::pa_context_errno(context_.get())));
		}

		if (::pa_threaded_mainloop_start(mainloop_.get()) < 0) {
			Throw<PlatformException>("PulseAudio notification mainloop start failed.");
		}
		is_mainloop_started_ = true;

		{
			const PulseThreadedMainloopLock lock(mainloop_.get());

			while (true) {
				const auto state = ::pa_context_get_state(context_.get());
				if (state == PA_CONTEXT_READY) {
					break;
				}
				if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
					const auto* error = ::pa_strerror(::pa_context_errno(context_.get()));
					Throw<PlatformException>("PulseAudio notification context failed: {}", error);
				}
				::pa_threaded_mainloop_wait(mainloop_.get());
			}

			::pa_context_set_subscribe_callback(context_.get(), &PulseDeviceStateNotification::SubscriptionCallback, this);
			PulseOperationPtr operation(::pa_context_subscribe(context_.get(),
				static_cast<pa_subscription_mask_t>(PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SERVER),
				nullptr,
				nullptr));
		}

		XAMP_LOG_D(logger_, "PulseAudio device state notification started.");
	}
	catch (const std::exception& e) {
		XAMP_LOG_D(logger_, "PulseAudio device state notification disabled: {}", e.what());
		Stop();
	}
}

void PulseDeviceStateNotification::Stop() noexcept {
	is_running_ = false;

	if (context_) {
		::pa_context_set_state_callback(context_.get(), nullptr, nullptr);
		::pa_context_set_subscribe_callback(context_.get(), nullptr, nullptr);
	}

	if (mainloop_ && is_mainloop_started_.exchange(false)) {
		::pa_threaded_mainloop_stop(mainloop_.get());
	}

	context_.reset();

	if (mainloop_) {
		mainloop_.reset();
	}
}

void PulseDeviceStateNotification::Notify(DeviceState state, std::string device_id) {
	if (auto callback = callback_.lock()) {
		callback->OnDeviceStateChange(state, device_id);
	}
}

void PulseDeviceStateNotification::OnContextStateChanged(pa_context* context) {
	if (mainloop_ == nullptr) {
		return;
	}

	const auto state = ::pa_context_get_state(context);
	if (state == PA_CONTEXT_READY || state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
		::pa_threaded_mainloop_signal(mainloop_.get(), 0);
	}
}

void PulseDeviceStateNotification::OnSubscriptionEvent(pa_context* context,
	pa_subscription_event_type_t event_type,
	uint32_t index) {
	const auto facility = event_type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
	const auto type = event_type & PA_SUBSCRIPTION_EVENT_TYPE_MASK;

	if (facility == PA_SUBSCRIPTION_EVENT_SERVER) {
		PulseOperationPtr operation(::pa_context_get_server_info(context,
			&PulseDeviceStateNotification::ServerInfoCallback,
			this));
		return;
	}

	if (facility != PA_SUBSCRIPTION_EVENT_SINK) {
		return;
	}

	switch (type) {
	case PA_SUBSCRIPTION_EVENT_NEW:
	case PA_SUBSCRIPTION_EVENT_CHANGE: {
		PulseOperationPtr operation(::pa_context_get_sink_info_by_index(context,
			index,
			&PulseDeviceStateNotification::SinkInfoCallback,
			this));
		break;
	}
	case PA_SUBSCRIPTION_EVENT_REMOVE:
		Notify(DeviceState::DEVICE_STATE_REMOVED, std::to_string(index));
		break;
	default:
		break;
	}
}

void PulseDeviceStateNotification::OnServerInfo(const pa_server_info* info) {
	if (info == nullptr || info->default_sink_name == nullptr || info->default_sink_name[0] == '\0') {
		Notify(DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE, {});
		return;
	}
	Notify(DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE, info->default_sink_name);
}

void PulseDeviceStateNotification::OnSinkInfo(const pa_sink_info* info, DeviceState state, uint32_t index) {
	if (info == nullptr) {
		Notify(state, std::to_string(index));
		return;
	}
	Notify(state, info->name != nullptr ? info->name : std::to_string(index));
}

void PulseDeviceStateNotification::ContextStateCallback(pa_context* context, void* userdata) {
	if (userdata == nullptr) {
		return;
	}
	static_cast<PulseDeviceStateNotification*>(userdata)->OnContextStateChanged(context);
}

void PulseDeviceStateNotification::SubscriptionCallback(pa_context* context,
	pa_subscription_event_type_t event_type,
	uint32_t index,
	void* userdata) {
	if (userdata == nullptr) {
		return;
	}
	static_cast<PulseDeviceStateNotification*>(userdata)->OnSubscriptionEvent(context, event_type, index);
}

void PulseDeviceStateNotification::ServerInfoCallback(pa_context*,
	const pa_server_info* info,
	void* userdata) {
	if (userdata == nullptr) {
		return;
	}
	static_cast<PulseDeviceStateNotification*>(userdata)->OnServerInfo(info);
}

void PulseDeviceStateNotification::SinkInfoCallback(pa_context*,
	const pa_sink_info* info,
	int eol,
	void* userdata) {
	if (userdata == nullptr || eol != 0) {
		return;
	}
	static_cast<PulseDeviceStateNotification*>(userdata)->OnSinkInfo(info,
		DeviceState::DEVICE_STATE_ADDED,
		info != nullptr ? info->index : PA_INVALID_INDEX);
}

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
