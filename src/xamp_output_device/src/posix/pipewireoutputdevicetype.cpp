//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <output_device/posix/pipewireoutputdevicetype.h>

#include <output_device/posix/pipewire_private.h>
#include <output_device/posix/pipewireoutputdevice.h>

#include <algorithm>
#include <string>
#include <string_view>

#include <pipewire/keys.h>
#include <spa/utils/dict.h>

#include <base/unique_handle.h>
#include <base/exception.h>
#include <base/str_utilts.h>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

namespace {

constexpr std::wstring_view kDefaultDeviceName = L"PipeWire Default Sink";
constexpr std::string_view kDefaultDeviceId = "default";
constexpr uint32_t kFallbackSampleRate = 48000;
constexpr std::string_view kAudioSink = "Audio/Sink";
constexpr std::string_view kDummyOutput = "Dummy Output";

bool IsUsablePipeWireSink(const char* name, const char* description) noexcept {
	if (name == nullptr || name[0] == '\0') {
		return false;
	}

	const std::string_view sink_name(name);
	if (sink_name == "auto_null") {
		return false;
	}

	if (description != nullptr && std::string_view(description) == kDummyOutput) {
		return false;
	}

	return true;
}

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
	info.device_type_id = XAMP_UUID_OF(PipeWireOutputDeviceType);
	info.desc = PipeWireOutputDeviceType::Description;
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

std::wstring ToDeviceName(const char* description, const char* name) {
	if (description != nullptr && description[0] != '\0') {
		return String::ToStdWString(description);
	}
	if (name != nullptr && name[0] != '\0') {
		return String::ToStdWString(name);
	}
	return std::wstring(kDefaultDeviceName);
}

class PipeWireDeviceScanner final {
public:
	std::vector<DeviceInfo> Scan() {
		EnsurePipeWireInitialized();

		loop_.reset(::pw_thread_loop_new("xamp-pipewire-scan", nullptr));
		if (loop_ == nullptr) {
			Throw<PlatformException>("PipeWire scan thread loop create failed.");
		}

		context_.reset(::pw_context_new(::pw_thread_loop_get_loop(loop_.get()), nullptr, 0));
		if (context_ == nullptr) {
			Throw<PlatformException>("PipeWire scan context create failed.");
		}

		core_.reset(::pw_context_connect(context_.get(), nullptr, 0));
		if (core_ == nullptr) {
			Throw<PlatformException>("PipeWire scan core connect failed.");
		}

		static constexpr pw_core_events core_events{
			.version = PW_VERSION_CORE_EVENTS,
			.done = &PipeWireDeviceScanner::coreDoneCallback,
		};
		pw_core_add_listener(core_.get(), &core_listener_, &core_events, this);

		registry_.reset(::pw_core_get_registry(core_.get(), PW_VERSION_REGISTRY, 0));
		if (registry_ == nullptr) {
			Throw<PlatformException>("PipeWire registry create failed.");
		}

		static constexpr pw_registry_events registry_events{
			.version = PW_VERSION_REGISTRY_EVENTS,
			.global = &PipeWireDeviceScanner::RegistryGlobalCallback,
		};
		pw_registry_add_listener(registry_.get(), &registry_listener_, &registry_events, this);

		core_sync_seq_ = pw_core_sync(core_.get(), PW_ID_CORE, 0);

		if (::pw_thread_loop_start(loop_.get()) != 0) {
			Throw<PlatformException>("PipeWire scan thread loop start failed.");
		}

		{
			const PipeWireThreadLoopLock lock(loop_.get());
			while (!done_) {
				::pw_thread_loop_wait(loop_.get());
			}
		}

		stop();

		std::ranges::sort(devices_, [](const auto& lhs, const auto& rhs) {
			return lhs.name < rhs.name;
		});
		devices_.insert(devices_.begin(), MakeFallbackDeviceInfo());
		return devices_;
	}

	~PipeWireDeviceScanner() {
		stop();
	}

private:
	static void coreDoneCallback(void* userdata, uint32_t id, int seq) {
		auto* self = static_cast<PipeWireDeviceScanner*>(userdata);
		if (self == nullptr || id != PW_ID_CORE || seq != self->core_sync_seq_) {
			return;
		}
		self->done_ = true;
		if (self->loop_ != nullptr) {
			::pw_thread_loop_signal(self->loop_.get(), false);
		}
	}

	static void RegistryGlobalCallback(void* userdata,
		uint32_t,
		uint32_t,
		const char* type,
		uint32_t,
		const spa_dict* props) {
		auto* self = static_cast<PipeWireDeviceScanner*>(userdata);
		if (self == nullptr || type == nullptr || props == nullptr) {
			return;
		}

		if (std::string_view(type) != PW_TYPE_INTERFACE_Node) {
			return;
		}

		const auto* media_class = ::spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
		if (media_class == nullptr || std::string_view(media_class) != kAudioSink) {
			return;
		}

		const auto* name = ::spa_dict_lookup(props, PW_KEY_NODE_NAME);
		const auto* description = ::spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
		if (!IsUsablePipeWireSink(name, description)) {
			return;
		}

		self->devices_.push_back(MakeDeviceInfo(name,
			ToDeviceName(description, name),
			false,
			AudioFormat::kMaxChannel,
			kFallbackSampleRate));
	}

	void stop() noexcept {
		if (loop_ != nullptr) {
			{
				const PipeWireThreadLoopLock lock(loop_.get());
				if (registry_ != nullptr) {
					::spa_hook_remove(&registry_listener_);
					registry_.reset();
				}
				if (core_ != nullptr) {
					::spa_hook_remove(&core_listener_);
				}
				core_.reset();
				context_.reset();
			}
			::pw_thread_loop_stop(loop_.get());
			loop_.reset();
		}
	}

	PipeWireThreadLoopPtr loop_;
	PipeWireContextPtr context_;
	PipeWireCorePtr core_;
	PipeWireRegistryPtr registry_;
	spa_hook core_listener_{};
	spa_hook registry_listener_{};
	int core_sync_seq_{ 0 };
	bool done_{ false };
	std::vector<DeviceInfo> devices_;
};

std::vector<DeviceInfo> EnumeratePipeWireSinks() {
	PipeWireDeviceScanner scanner;
	return scanner.Scan();
}
}

class PipeWireOutputDeviceType::PipeWireOutputDeviceTypeImpl final {
public:
	PipeWireOutputDeviceTypeImpl()
		: logger_(XampLoggerFactory.getLogger(XAMP_LOG_NAME(PipeWireOutputDeviceType))) {
		scanNewDevice();
	}

	void scanNewDevice() {
		try {
			devices_ = EnumeratePipeWireSinks();
		}
		catch (const std::exception& e) {
			XAMP_LOG_D(logger_, "PipeWire scan failed: {}.", e.what());
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
		return MakeAlign<IOutputDevice, PipeWireOutputDevice>(thread_pool, device_id);
	}

private:
	LoggerPtr logger_;
	std::vector<DeviceInfo> devices_;
};

PipeWireOutputDeviceType::PipeWireOutputDeviceType()
	: impl_(MakeAlign<PipeWireOutputDeviceTypeImpl>()) {
}

void PipeWireOutputDeviceType::scanNewDevice() {
	impl_->scanNewDevice();
}

size_t PipeWireOutputDeviceType::getDeviceCount() const {
	return impl_->getDeviceCount();
}

DeviceInfo PipeWireOutputDeviceType::getDeviceInfo(uint32_t device) const {
	return impl_->getDeviceInfo(device);
}

std::vector<DeviceInfo> PipeWireOutputDeviceType::getDeviceInfo() const {
	return impl_->getDeviceInfo();
}

std::optional<DeviceInfo> PipeWireOutputDeviceType::getDefaultDeviceInfo() const {
	return impl_->getDefaultDeviceInfo();
}

ScopedPtr<IOutputDevice> PipeWireOutputDeviceType::makeDevice(const std::shared_ptr<xamp::base::IThreadPool>& thread_pool,
	const std::string& device_id) {
	return impl_->makeDevice(thread_pool, device_id);
}

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
