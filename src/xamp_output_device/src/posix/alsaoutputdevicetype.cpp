//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <output_device/posix/alsaoutputdevicetype.h>

#include <output_device/posix/alsaoutputdevice.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

#include <alsa/asoundlib.h>

#include <base/unique_handle.h>
#include <base/logger.h>
#include <base/str_utilts.h>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

namespace {
using namespace std::literals;

constexpr std::wstring_view kDefaultDeviceName = L"ALSA Default PCM";
constexpr std::string_view kDefaultDeviceId = "default";
constexpr uint32_t kFallbackSampleRate = 48000;
constexpr int kAlsaOpenMode = SND_PCM_NONBLOCK;

constexpr std::array kProbeSampleRates{
	8000u,
	16000u,
	22050u,
	32000u,
	44100u,
	48000u,
	88200u,
	96000u,
	176400u,
	192000u,
};

struct AlsaControlTraits final {
	using handle_type = snd_ctl_t*;

	static handle_type invalid() noexcept {
		return nullptr;
	}

	static void close(handle_type handle) noexcept {
		if (handle != nullptr) {
			::snd_ctl_close(handle);
			::snd_config_update_free_global();
		}
	}
};

using AlsaControlHandle = UniqueHandle<snd_ctl_t*, AlsaControlTraits>;

struct AlsaPcmTraits final {
	using handle_type = snd_pcm_t*;
	
	static handle_type invalid() noexcept {
		return nullptr;
	}

	static void close(handle_type handle) noexcept {
		if (handle != nullptr) {
			::snd_pcm_close(handle);
			::snd_config_update_free_global();
		}
	}
};

using AlsaPcmHandle = UniqueHandle<snd_pcm_t*, AlsaPcmTraits>;

struct AlsaPlaybackProbe final {
	uint16_t channels{ AudioFormat::kMaxChannel };
	uint32_t sample_rate{ kFallbackSampleRate };
};

DeviceInfo MakeDeviceInfo(std::string device_id,
	std::wstring device_name,
	bool is_default_device,
	uint16_t channels,
	uint32_t sample_rate) {
	DeviceInfo info;
	info.is_default_device = is_default_device;
	info.is_hardware_control_volume = false;
	info.is_normalized_volume = true;
	info.connect_type = DeviceConnectType::UNKNOWN;
	info.name = std::move(device_name);
	info.device_id = std::move(device_id);
	info.device_type_id = XAMP_UUID_OF(AlsaOutputDeviceType);
	info.desc = AlsaOutputDeviceType::Description;
	info.default_format = AudioFormat(DataFormat::FORMAT_PCM,
		channels,
		ByteFormat::FLOAT32,
		sample_rate);
	return info;
}

DeviceInfo MakeFallbackDeviceInfo() {
	return MakeDeviceInfo(std::string(kDefaultDeviceId),
		std::wstring(kDefaultDeviceName),
		true,
		AudioFormat::kMaxChannel,
		kFallbackSampleRate);
}

[[nodiscard]] bool IsFloatPlaybackSupported(snd_pcm_t* handle, snd_pcm_hw_params_t* params) {
	return ::snd_pcm_hw_params_test_format(handle, params, SND_PCM_FORMAT_FLOAT_LE) == 0
		|| ::snd_pcm_hw_params_test_format(handle, params, SND_PCM_FORMAT_FLOAT) == 0;
}

[[nodiscard]] uint32_t ProbePreferredSampleRate(snd_pcm_t* handle, snd_pcm_hw_params_t* params) {
	uint32_t preferred_sample_rate{ 0 };
	uint32_t fallback_sample_rate{ 0 };

	for (const auto sample_rate : kProbeSampleRates) {
		if (::snd_pcm_hw_params_test_rate(handle, params, sample_rate, 0) != 0) {
			continue;
		}

		if (fallback_sample_rate == 0) {
			fallback_sample_rate = sample_rate;
		}

		if (sample_rate <= kFallbackSampleRate) {
			preferred_sample_rate = sample_rate;
		}
	}

	if (preferred_sample_rate != 0) {
		return preferred_sample_rate;
	}

	if (fallback_sample_rate != 0) {
		return fallback_sample_rate;
	}

	return kFallbackSampleRate;
}

[[nodiscard]] std::optional<AlsaPlaybackProbe> ProbeAlsaPlaybackDevice(const std::string& device_id) {
	snd_pcm_t* pcm_handle{ nullptr };
	const auto open_error = ::snd_pcm_open(&pcm_handle,
		device_id.c_str(),
		SND_PCM_STREAM_PLAYBACK,
		kAlsaOpenMode);
	if (open_error < 0) {
		return std::nullopt;
	}

	AlsaPcmHandle pcm(pcm_handle);

	snd_pcm_hw_params_t* params{ nullptr };
	snd_pcm_hw_params_alloca(&params);

	if (::snd_pcm_hw_params_any(pcm.get(), params) < 0) {
		return std::nullopt;
	}

	unsigned int max_channels{ 0 };
	if (::snd_pcm_hw_params_get_channels_max(params, &max_channels) < 0 || max_channels == 0) {
		return std::nullopt;
	}

	if (!IsFloatPlaybackSupported(pcm.get(), params)) {
		return std::nullopt;
	}

	AlsaPlaybackProbe probe;
	probe.channels = static_cast<uint16_t>((std::min)(static_cast<uint32_t>(max_channels), AudioFormat::kMaxChannel));
	probe.sample_rate = ProbePreferredSampleRate(pcm.get(), params);
	return probe;
}

[[nodiscard]] std::string FormatAlsaDeviceId(const std::string & card_id, int device) {
	return String::Format("plughw:{},{}", card_id, device);
}

[[nodiscard]] std::wstring FormatAlsaDeviceName(const std::string& card_name, const char* pcm_id) {
	auto name = String::Format("{} ({})", card_name, pcm_id);
	return String::ToStdWString(name);
}

std::vector<DeviceInfo> EnumerateAlsaPlaybackDevices() {
	std::vector<DeviceInfo> devices;
	devices.push_back(MakeFallbackDeviceInfo());

	std::unordered_set<std::string> device_ids;
	std::unordered_set<std::wstring> device_names;
	device_ids.emplace(std::string(kDefaultDeviceId));
	device_names.emplace(std::wstring(kDefaultDeviceName));

	snd_ctl_card_info_t* control_info{ nullptr };
	snd_pcm_info_t* pcm_info{ nullptr };
	snd_ctl_card_info_alloca(&control_info);
	snd_pcm_info_alloca(&pcm_info);

	int card{ -1 };
	if (::snd_card_next(&card) < 0) {
		return devices;
	}

	while (card >= 0) {
		auto control_name = String::Format("hw:{}", card);

		snd_ctl_t* control_handle{ nullptr };
		if (::snd_ctl_open(&control_handle, control_name.c_str(), 0) < 0) {
			::snd_card_next(&card);
			continue;
		}

		AlsaControlHandle control(control_handle);
		if (::snd_ctl_card_info(control.get(), control_info) < 0) {
			::snd_card_next(&card);
			continue;
		}

		int device{ -1 };
		while (::snd_ctl_pcm_next_device(control.get(), &device) >= 0 && device >= 0) {
			::snd_pcm_info_set_device(pcm_info, device);
			::snd_pcm_info_set_subdevice(pcm_info, 0);
			::snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_PLAYBACK);

			if (::snd_ctl_pcm_info(control.get(), pcm_info) < 0) {
				continue;
			}

			auto device_id = FormatAlsaDeviceId(::snd_ctl_card_info_get_id(control_info), device);
			if (!device_ids.emplace(device_id).second) {
				continue;
			}

			auto display_name = FormatAlsaDeviceName(::snd_ctl_card_info_get_name(control_info),
				::snd_pcm_info_get_id(pcm_info));
			auto probe = ProbeAlsaPlaybackDevice(device_id);
			if (!probe) {
				continue;
			}

			if (!device_names.emplace(display_name).second) {
				continue;
			}

			devices.push_back(MakeDeviceInfo(std::move(device_id),
				std::move(display_name),
				false,
				probe->channels,
				probe->sample_rate));
		}

		::snd_card_next(&card);
	}

	return devices;
}
}

class AlsaOutputDeviceType::AlsaOutputDeviceTypeImpl final {
public:
	AlsaOutputDeviceTypeImpl()
		: logger_(XampLoggerFactory.getLogger(XAMP_LOG_NAME(AlsaOutputDeviceType))) {
		scanNewDevice();
	}

	void scanNewDevice() {
		try {
			devices_ = EnumerateAlsaPlaybackDevices();
		}
		catch (const std::exception& e) {
			XAMP_LOG_D(logger_, "ALSA scan failed: {}. Use default PCM fallback.", e.what());
			devices_.clear();
		}

		if (devices_.empty()) {
			devices_.push_back(MakeFallbackDeviceInfo());
		}
	}

	[[nodiscard]] size_t getDeviceCount() const {
		return devices_.size();
	}

	[[nodiscard]] DeviceInfo getDeviceInfo(uint32_t device) const {
		return devices_.at(device);
	}

	[[nodiscard]] std::vector<DeviceInfo> getDeviceInfo() const {
		return devices_;
	}

	[[nodiscard]] std::optional<DeviceInfo> getDefaultDeviceInfo() const {
		if (devices_.empty()) {
			return std::nullopt;
		}
		return devices_.front();
	}

	ScopedPtr<IOutputDevice> makeDevice(const std::shared_ptr<xamp::base::IThreadPool>& thread_pool,
		const std::string& device_id) {
		return MakeAlign<IOutputDevice, AlsaOutputDevice>(thread_pool, device_id);
	}

private:
	LoggerPtr logger_;
	std::vector<DeviceInfo> devices_;
};

AlsaOutputDeviceType::AlsaOutputDeviceType()
	: impl_(MakeAlign<AlsaOutputDeviceTypeImpl>()) {
}

void AlsaOutputDeviceType::scanNewDevice() {
	impl_->scanNewDevice();
}

size_t AlsaOutputDeviceType::getDeviceCount() const {
	return impl_->getDeviceCount();
}

DeviceInfo AlsaOutputDeviceType::getDeviceInfo(uint32_t device) const {
	return impl_->getDeviceInfo(device);
}

std::vector<DeviceInfo> AlsaOutputDeviceType::getDeviceInfo() const {
	return impl_->getDeviceInfo();
}

std::optional<DeviceInfo> AlsaOutputDeviceType::getDefaultDeviceInfo() const {
	return impl_->getDefaultDeviceInfo();
}

ScopedPtr<IOutputDevice> AlsaOutputDeviceType::makeDevice(const std::shared_ptr<xamp::base::IThreadPool>& thread_pool,
	const std::string& device_id) {
	return impl_->makeDevice(thread_pool, device_id);
}

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
