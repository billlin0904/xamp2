//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <output_device/posix/pulseoutputdevicetype.h>

#include <output_device/posix/pulseoutputdevice.h>
#include <output_device/posix/pulse_private.h>

#include <chrono>
#include <thread>

#include <pulse/pulseaudio.h>

#include <base/exception.h>
#include <base/str_utilts.h>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

namespace {
constexpr std::wstring_view kDefaultDeviceName = L"PulseAudio Default Sink";
constexpr std::string_view kDefaultDeviceId = "default";
constexpr uint32_t kFallbackSampleRate = 48000;

DeviceInfo MakeFallbackDeviceInfo() {
	DeviceInfo info;
	info.is_default_device = true;
	info.is_hardware_control_volume = false;
	info.is_normalized_volume = true;
	info.connect_type = DeviceConnectType::UNKNOWN;
	info.name = kDefaultDeviceName;
	info.device_id = kDefaultDeviceId;
	info.device_type_id = XAMP_UUID_OF(PulseOutputDeviceType);
	info.desc = PulseOutputDeviceType::Description;
	info.default_format = AudioFormat(DataFormat::FORMAT_PCM,
		AudioFormat::kMaxChannel,
		ByteFormat::FLOAT32,
		kFallbackSampleRate);
	return info;
}

std::wstring ToDeviceName(const char* value, const char* fallback) {
	if (value != nullptr && value[0] != '\0') {
		return String::ToStdWString(value);
	}
	if (fallback != nullptr && fallback[0] != '\0') {
		return String::ToStdWString(fallback);
	}
	return std::wstring(kDefaultDeviceName);
}

void IterateMainloop(pa_mainloop* mainloop) {
	int result = 0;
	if (::pa_mainloop_iterate(mainloop, 1, &result) < 0 || result < 0) {
		Throw<PlatformException>("PulseAudio mainloop iterate failed.");
	}
}

void WaitForContextReady(pa_mainloop* mainloop, pa_context* context) {
	while (true) {
		switch (::pa_context_get_state(context)) {
		case PA_CONTEXT_READY:
			return;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			Throw<PlatformException>("PulseAudio context failed: {}",
				::pa_strerror(::pa_context_errno(context)));
		default:
			IterateMainloop(mainloop);
			break;
		}
	}
}

void WaitForOperation(pa_mainloop* mainloop, pa_operation* operation) {
	if (operation == nullptr) {
		Throw<PlatformException>("PulseAudio operation create failed.");
	}
	while (::pa_operation_get_state(operation) == PA_OPERATION_RUNNING) {
		IterateMainloop(mainloop);
	}
}

struct ServerInfoState {
	std::string default_sink_name;
	bool done{ false };
};

void ServerInfoCallback(pa_context*, const pa_server_info* info, void* userdata) {
	auto* state = static_cast<ServerInfoState*>(userdata);
	if (info != nullptr && info->default_sink_name != nullptr) {
		state->default_sink_name = info->default_sink_name;
	}
	state->done = true;
}

struct SinkListState {
	std::string default_sink_name;
	std::vector<DeviceInfo> devices;
	bool done{ false };
};

void SinkInfoCallback(pa_context*, const pa_sink_info* info, int eol, void* userdata) {
	auto* state = static_cast<SinkListState*>(userdata);
	if (eol > 0) {
		state->done = true;
		return;
	}
	if (eol < 0 || info == nullptr) {
		return;
	}

	DeviceInfo device_info;
	device_info.is_default_device = state->default_sink_name == info->name;
	device_info.is_hardware_control_volume = false;
	device_info.is_normalized_volume = true;
	device_info.connect_type = DeviceConnectType::UNKNOWN;
	device_info.name = ToDeviceName(info->description, info->name);
	device_info.device_id = info->name != nullptr ? info->name : std::string(kDefaultDeviceId);
	device_info.device_type_id = XAMP_UUID_OF(PulseOutputDeviceType);
	device_info.desc = PulseOutputDeviceType::Description;
	device_info.default_format = AudioFormat(DataFormat::FORMAT_PCM,
		std::max<uint8_t>(info->sample_spec.channels, 1),
		ByteFormat::FLOAT32,
		info->sample_spec.rate != 0 ? info->sample_spec.rate : kFallbackSampleRate);

	state->devices.push_back(std::move(device_info));
}

std::vector<DeviceInfo> EnumeratePulseSinks() {
	PulseMainloopPtr mainloop(::pa_mainloop_new());
	if (!mainloop) {
		Throw<PlatformException>("PulseAudio mainloop create failed.");
	}

	auto* api = ::pa_mainloop_get_api(mainloop.get());
	PulseContextPtr context(::pa_context_new(api, "XAMP device scan"));
	if (!context) {
		Throw<PlatformException>("PulseAudio context create failed.");
	}

	if (::pa_context_connect(context.get(), nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
		Throw<PlatformException>("PulseAudio connect failed: {}",
			::pa_strerror(::pa_context_errno(context.get())));
	}

	WaitForContextReady(mainloop.get(), context.get());

	ServerInfoState server_state;
	{
		PulseOperationPtr operation(::pa_context_get_server_info(context.get(),
			ServerInfoCallback,
			&server_state));
		WaitForOperation(mainloop.get(), operation.get());
	}

	SinkListState sink_state;
	sink_state.default_sink_name = std::move(server_state.default_sink_name);
	{
		PulseOperationPtr operation(::pa_context_get_sink_info_list(context.get(),
			SinkInfoCallback,
			&sink_state));
		WaitForOperation(mainloop.get(), operation.get());
	}

	if (!sink_state.default_sink_name.empty()) {
		const auto default_itr = std::ranges::find_if(sink_state.devices,
			[&sink_state](const auto& device) {
				return device.device_id == sink_state.default_sink_name;
			});
		if (default_itr != sink_state.devices.end()) {
			std::rotate(sink_state.devices.begin(), default_itr, default_itr + 1);
		}
	}

	return sink_state.devices;
}
}

class PulseOutputDeviceType::PulseOutputDeviceTypeImpl final {
public:
	PulseOutputDeviceTypeImpl()
		: logger_(XampLoggerFactory.GetLogger(XAMP_LOG_NAME(PulseOutputDeviceType))) {
		ScanNewDevice();
	}

	void ScanNewDevice() {
		try {
			devices_ = EnumeratePulseSinks();
		}
		catch (const std::exception& e) {
			XAMP_LOG_D(logger_, "PulseAudio scan failed: {}. Use default sink fallback.", e.what());
			devices_.clear();
		}

		if (devices_.empty()) {
			devices_.push_back(MakeFallbackDeviceInfo());
		}
	}

	[[nodiscard]] size_t GetDeviceCount() const {
		return devices_.size();
	}

	[[nodiscard]] DeviceInfo GetDeviceInfo(uint32_t device) const {
		return devices_.at(device);
	}

	[[nodiscard]] std::vector<DeviceInfo> GetDeviceInfo() const {
		return devices_;
	}

	[[nodiscard]] std::optional<DeviceInfo> GetDefaultDeviceInfo() const {
		if (devices_.empty()) {
			return std::nullopt;
		}
		return devices_.front();
	}

	ScopedPtr<IOutputDevice> MakeDevice(const std::shared_ptr<xamp::base::IThreadPoolExecutor>& thread_pool,
		const std::string& device_id) {
		return MakeAlign<IOutputDevice, PulseOutputDevice>(thread_pool, device_id);
	}

private:
	LoggerPtr logger_;
	std::vector<DeviceInfo> devices_;
};

PulseOutputDeviceType::PulseOutputDeviceType()
	: impl_(MakeAlign<PulseOutputDeviceTypeImpl>()) {
}

void PulseOutputDeviceType::ScanNewDevice() {
	impl_->ScanNewDevice();
}

size_t PulseOutputDeviceType::GetDeviceCount() const {
	return impl_->GetDeviceCount();
}

DeviceInfo PulseOutputDeviceType::GetDeviceInfo(uint32_t device) const {
	return impl_->GetDeviceInfo(device);
}

std::vector<DeviceInfo> PulseOutputDeviceType::GetDeviceInfo() const {
	return impl_->GetDeviceInfo();
}

std::optional<DeviceInfo> PulseOutputDeviceType::GetDefaultDeviceInfo() const {
	return impl_->GetDefaultDeviceInfo();
}

ScopedPtr<IOutputDevice> PulseOutputDeviceType::MakeDevice(const std::shared_ptr<xamp::base::IThreadPoolExecutor>& thread_pool,
	const std::string& device_id) {
	return impl_->MakeDevice(thread_pool, device_id);
}

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
