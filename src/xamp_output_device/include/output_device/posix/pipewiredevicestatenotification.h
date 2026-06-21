//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <unordered_map>

#include <base/logger.h>
#include <base/memory.h>

#include <output_device/idevicestatelistener.h>
#include <output_device/idevicestatenotification.h>
#include <output_device/posix/pipewire_private.h>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(PipeWireDeviceStateNotification);

class PipeWireDeviceStateNotification final : public IDeviceStateNotification {
public:
	explicit PipeWireDeviceStateNotification(std::weak_ptr<IDeviceStateListener> callback);

	~PipeWireDeviceStateNotification() override;

	void run() override;

private:
	void stop() noexcept;

	void notify(DeviceState state, std::string device_id);

	[[nodiscard]] bool rememberSink(uint32_t id, const spa_dict* props);

	static void coreDoneCallback(void* userdata, uint32_t id, int seq);

	static void RegistryGlobalCallback(void* userdata,
		uint32_t id,
		uint32_t permissions,
		const char* type,
		uint32_t version,
		const spa_dict* props);

	static void registryGlobalRemoveCallback(void* userdata, uint32_t id);

	std::weak_ptr<IDeviceStateListener> callback_;
	LoggerPtr logger_;
	PipeWireThreadLoopPtr loop_;
	PipeWireContextPtr context_;
	PipeWireCorePtr core_;
	PipeWireRegistryPtr registry_;
	spa_hook core_listener_{};
	spa_hook registry_listener_{};
	std::unordered_map<uint32_t, std::string> sinks_;
	int core_sync_seq_{ 0 };
	bool core_ready_{ false };
	std::atomic<bool> is_running_{ false };
};

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
