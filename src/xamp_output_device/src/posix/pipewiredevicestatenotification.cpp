//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <output_device/posix/pipewiredevicestatenotification.h>

#include <pipewire/keys.h>
#include <spa/utils/dict.h>

#include <base/exception.h>

#include <string_view>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

PipeWireDeviceStateNotification::PipeWireDeviceStateNotification(std::weak_ptr<IDeviceStateListener> callback)
	: callback_(std::move(callback))
	, logger_(XampLoggerFactory.getLogger(XAMP_LOG_NAME(PipeWireDeviceStateNotification))) {
}

PipeWireDeviceStateNotification::~PipeWireDeviceStateNotification() {
	stop();
}

void PipeWireDeviceStateNotification::run() {
	if (is_running_.exchange(true)) {
		return;
	}

	try {
		EnsurePipeWireInitialized();

		loop_.reset(pw_thread_loop_new("xamp-pipewire-device-notification", nullptr));
		if (loop_ == nullptr) {
			Throw<PlatformException>("PipeWire notification thread loop create failed.");
		}

		context_.reset(pw_context_new(pw_thread_loop_get_loop(loop_.get()), nullptr, 0));
		if (context_ == nullptr) {
			Throw<PlatformException>("PipeWire notification context create failed.");
		}

		core_.reset(pw_context_connect(context_.get(), nullptr, 0));
		if (core_ == nullptr) {
			Throw<PlatformException>("PipeWire notification core connect failed.");
		}

		static constexpr pw_core_events core_events{
			.version = PW_VERSION_CORE_EVENTS,
			.done = &PipeWireDeviceStateNotification::coreDoneCallback,
		};
		pw_core_add_listener(core_.get(), &core_listener_, &core_events, this);

		registry_.reset(pw_core_get_registry(core_.get(), PW_VERSION_REGISTRY, 0));
		if (registry_ == nullptr) {
			Throw<PlatformException>("PipeWire notification registry create failed.");
		}

		static constexpr pw_registry_events registry_events{
			.version = PW_VERSION_REGISTRY_EVENTS,
			.global = &PipeWireDeviceStateNotification::RegistryGlobalCallback,
			.global_remove = &PipeWireDeviceStateNotification::registryGlobalRemoveCallback,
		};
		pw_registry_add_listener(registry_.get(), &registry_listener_, &registry_events, this);

		core_sync_seq_ = pw_core_sync(core_.get(), PW_ID_CORE, 0);

		if (pw_thread_loop_start(loop_.get()) != 0) {
			Throw<PlatformException>("PipeWire notification thread loop start failed.");
		}

		{
			const PipeWireThreadLoopLock lock(loop_.get());
			while (!core_ready_) {
				pw_thread_loop_wait(loop_.get());
			}
		}

		XAMP_LOG_D(logger_, "PipeWire device state notification started.");
	}
	catch (const std::exception& e) {
		XAMP_LOG_D(logger_, "PipeWire device state notification disabled: {}", e.what());
		stop();
	}
}

void PipeWireDeviceStateNotification::stop() noexcept {
	is_running_ = false;

	if (loop_ != nullptr) {
		{
			const PipeWireThreadLoopLock lock(loop_.get());
			if (registry_ != nullptr) {
				spa_hook_remove(&registry_listener_);
				registry_.reset();
			}
			if (core_ != nullptr) {
				spa_hook_remove(&core_listener_);
			}
			core_.reset();
			context_.reset();
		}
		pw_thread_loop_stop(loop_.get());
		loop_.reset();
	}
}

void PipeWireDeviceStateNotification::Notify(DeviceState state, std::string device_id) {
	if (auto callback = callback_.lock()) {
		callback->onDeviceStateChange(state, device_id);
	}
}

bool PipeWireDeviceStateNotification::rememberSink(uint32_t id, const spa_dict* props) {
	const auto* name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
	if (name == nullptr || name[0] == '\0') {
		return false;
	}

	const auto* description = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
	if (std::string_view(name) == "auto_null") {
		return false;
	}
	if (description != nullptr && std::string_view(description) == "Dummy Output") {
		return false;
	}

	sinks_[id] = name;
	return true;
}

void PipeWireDeviceStateNotification::coreDoneCallback(void* userdata, uint32_t id, int seq) {
	auto* self = static_cast<PipeWireDeviceStateNotification*>(userdata);
	if (self == nullptr || id != PW_ID_CORE || seq != self->core_sync_seq_) {
		return;
	}

	self->core_ready_ = true;
	if (self->loop_ != nullptr) {
		::pw_thread_loop_signal(self->loop_.get(), false);
	}
}

void PipeWireDeviceStateNotification::RegistryGlobalCallback(void* userdata,
	uint32_t id,
	uint32_t,
	const char* type,
	uint32_t,
	const spa_dict* props) {
	auto* self = static_cast<PipeWireDeviceStateNotification*>(userdata);
	if (self == nullptr || type == nullptr || props == nullptr) {
		return;
	}

	if (std::string_view(type) != PW_TYPE_INTERFACE_Node) {
		return;
	}

	const auto* media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
	if (media_class == nullptr || std::string_view(media_class) != "Audio/Sink") {
		return;
	}

	if (!self->rememberSink(id, props)) {
		return;
	}

	self->Notify(DeviceState::DEVICE_STATE_ADDED, self->sinks_.at(id));
}

void PipeWireDeviceStateNotification::registryGlobalRemoveCallback(void* userdata, uint32_t id) {
	auto* self = static_cast<PipeWireDeviceStateNotification*>(userdata);
	if (self == nullptr) {
		return;
	}

	const auto sink = self->sinks_.find(id);
	if (sink == self->sinks_.end()) {
		return;
	}

	const auto device_id = sink->second;
	self->sinks_.erase(sink);
	self->Notify(DeviceState::DEVICE_STATE_REMOVED, device_id);
}

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
