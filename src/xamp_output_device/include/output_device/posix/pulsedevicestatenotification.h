//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/logger.h>
#include <base/memory.h>

#include <output_device/idevicestatelistener.h>
#include <output_device/idevicestatenotification.h>
#include <output_device/posix/pulse_private.h>

#include <atomic>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(PulseDeviceStateNotification);

class PulseDeviceStateNotification final : public IDeviceStateNotification {
public:
	explicit PulseDeviceStateNotification(std::weak_ptr<IDeviceStateListener> callback);

	~PulseDeviceStateNotification() override;

	void Run() override;

private:
	void Stop() noexcept;

	void Notify(DeviceState state, std::string device_id);

	void OnContextStateChanged(pa_context* context);

	void OnSubscriptionEvent(pa_context* context, pa_subscription_event_type_t event_type, uint32_t index);

	void OnServerInfo(const pa_server_info* info);

	void OnSinkInfo(const pa_sink_info* info, DeviceState state, uint32_t index);

	static void ContextStateCallback(pa_context* context, void* userdata);

	static void SubscriptionCallback(pa_context* context,
		pa_subscription_event_type_t event_type,
		uint32_t index,
		void* userdata);

	static void ServerInfoCallback(pa_context* context, const pa_server_info* info, void* userdata);

	static void SinkInfoCallback(pa_context* context, const pa_sink_info* info, int eol, void* userdata);

	std::weak_ptr<IDeviceStateListener> callback_;
	LoggerPtr logger_;
	PulseThreadedMainloopPtr mainloop_;
	PulseContextPtr context_;
	std::atomic<bool> is_mainloop_started_{ false };
	std::atomic<bool> is_running_{ false };
};

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
