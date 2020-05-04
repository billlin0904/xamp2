#if ENABLE_ASIO

#include <asiodrivers.h>
#include <iasiodrv.h>

#include <base/memory.h>
#include <base/align_ptr.h>
#include <base/str_utilts.h>
#include <base/dataconverter.h>
#include <base/logger.h>
#include <base/platform_thread.h>

#ifdef XAMP_OS_WIN
#include <output_device/win32/mmcss.h>
#endif

#include <output_device/volume.h>
#include <output_device/asioexception.h>
#include <output_device/asiodevice.h>

namespace xamp::output_device {

using namespace win32;

struct AsioCallbackInfo {
	bool boost_priority{false};
	bool is_xrun{ false };
	AsioDevice* device{ nullptr };
	AlignPtr<AsioDrivers> drivers{};
	AudioConvertContext data_context{};
	ASIOCallbacks asio_callbacks{};	
	Mmcss mmcss;
	std::array<ASIOBufferInfo, XAMP_MAX_CHANNEL> buffer_infos;
	std::array<ASIOChannelInfo, XAMP_MAX_CHANNEL> channel_infos;
} callbackInfo;

static XAMP_ALWAYS_INLINE long GetLatencyMs(long latency, long sampleRate) noexcept {
	return (long((latency * 1000) / sampleRate));
}

constexpr int32_t MAX_CLOCK_SOURCE_SIZE = 32;

AsioDevice::AsioDevice(const std::string& device_id)
	: is_removed_driver_(true)
	, is_stopped_(true)
	, is_streaming_(false)
	, is_stop_streaming_(false)
	, sample_format_(DsdFormat::DSD_INT8MSB)
	, io_format_(AsioIoFormat::IO_FORMAT_PCM)
	, volume_(0)
	, buffer_size_(0)
	, buffer_bytes_(0)
	, played_bytes_(0)
	, device_id_(device_id)
	, clock_source_(MAX_CLOCK_SOURCE_SIZE)
	, callback_(nullptr) {
}

AsioDevice::~AsioDevice() {
	try {
		CloseStream();
	}
	catch (...) {
	}
}

bool AsioDevice::CanHardwareControlVolume() const {
	return false;
}

bool AsioDevice::IsMuted() const {
	return volume_ == 0;
}

uint32_t AsioDevice::GetBufferSize() const noexcept {
	return buffer_size_ * mix_format_.GetChannels();
}

void AsioDevice::SetSampleFormat(DsdFormat format) {
	sample_format_ = format;
}

DsdFormat AsioDevice::GetSampleFormat() const noexcept {
	return sample_format_;
}

void AsioDevice::SetIoFormat(AsioIoFormat format) {
	io_format_ = format;
}

AsioIoFormat AsioDevice::GetIoFormat() const {
	ASIOIoFormat asio_fomrmat{};
	AsioIfFailedThrow2(::ASIOFuture(kAsioGetIoFormat, &asio_fomrmat), ASE_SUCCESS);
	return (asio_fomrmat.FormatType == kASIOPCMFormat)
		? AsioIoFormat::IO_FORMAT_PCM : AsioIoFormat::IO_FORMAT_DSD;
}

bool AsioDevice::IsSupportDsdFormat() const {
	ASIOIoFormat asio_format{};
	asio_format.FormatType = kASIODSDFormat;
	auto error = ::ASIOFuture(kAsioCanDoIoFormat, &asio_format);
	return error == ASE_SUCCESS;
}

void AsioDevice::ReOpen() {
	if (!callbackInfo.drivers) {
		callbackInfo.drivers = MakeAlign<AsioDrivers>();
	}
	callbackInfo.drivers->removeCurrentDriver();
	if (!callbackInfo.drivers->loadDriver(const_cast<char*>(device_id_.c_str()))) {
		throw ASIOException(Errors::XAMP_ERROR_DEVICE_NOT_FOUND);
	}
	is_removed_driver_ = false;
}

std::tuple<int32_t, int32_t> AsioDevice::GetDeviceBufferSize() const {
	long min_size = 0;
	long max_size = 0;
	long prefer_size = 0;
	long granularity = 0;
	AsioIfFailedThrow(ASIOGetBufferSize(&min_size, &max_size, &prefer_size, &granularity));

	XAMP_LOG_INFO("min_size:{} max_size:{} prefer_size:{} granularity:{}",
		min_size,
		max_size,
		prefer_size,
		granularity);

	long buffer_size = 0;

	if (buffer_size == 0) {
		buffer_size = prefer_size;
	}
	else if (buffer_size < min_size) {
		buffer_size = min_size;
	}
	else if (buffer_size > max_size) {
		buffer_size = max_size;
	}
	else if (granularity == -1) {
		// Make sure bufferSize is a power of two.
		auto log2_of_min_size = 0;
		auto log2_of_max_size = 0;

		for (unsigned int i = 0; i < sizeof(long) * 8; i++) {
			if (min_size & (1 << i))
				log2_of_min_size = i;
			if (max_size & (1 << i))
				log2_of_max_size = i;
		}

		auto min_delta = std::abs(buffer_size - (1 << log2_of_min_size));
		auto min_delta_num = log2_of_min_size;

		for (auto i = log2_of_min_size + 1; i <= log2_of_max_size; i++) {
			const auto current_delta = std::abs(buffer_size - (1 << i));
			if (current_delta < min_delta) {
				min_delta = current_delta;
				min_delta_num = i;
			}
		}

		buffer_size = (1 << min_delta_num);
		if (buffer_size < min_size) {
			buffer_size = min_size;
		}
		else if (buffer_size > max_size) {
			buffer_size = max_size;
		}
	}
	else if (granularity != 0) {
		// Set to an even multiple of granularity, rounding up.
		buffer_size = (buffer_size + granularity - 1) / granularity * granularity;
	}
	return { prefer_size, buffer_size };
}

void AsioDevice::CreateBuffers(const AudioFormat& output_format) {
	const auto [prefer_size, buffer_size] = GetDeviceBufferSize();

	long num_channel = 0;
	for (auto& info : callbackInfo.buffer_infos) {
		info.isInput = ASIOFalse;
		info.channelNum = num_channel;
		info.buffers[0] = nullptr;
		info.buffers[1] = nullptr;
		++num_channel;
	}

	callbackInfo.asio_callbacks.bufferSwitch = OnBufferSwitchCallback;
	callbackInfo.asio_callbacks.sampleRateDidChange = OnSampleRateChangedCallback;
	callbackInfo.asio_callbacks.asioMessage = OnAsioMessagesCallback;
	callbackInfo.asio_callbacks.bufferSwitchTimeInfo = OnBufferSwitchTimeInfoCallback;
	callbackInfo.data_context.volume_factor = LinearToLog(volume_);

	auto result = ::ASIOCreateBuffers(callbackInfo.buffer_infos.data(),
		output_format.GetChannels(),
		buffer_size,
		&callbackInfo.asio_callbacks);
	if (result != ASE_OK) {
		AsioIfFailedThrow(::ASIOCreateBuffers(callbackInfo.buffer_infos.data(),
			output_format.GetChannels(),
			prefer_size,
			&callbackInfo.asio_callbacks));
		buffer_size_ = prefer_size;
	}
	else {
		buffer_size_ = buffer_size;
	}

	for (long i = 0; i < callbackInfo.buffer_infos.size(); ++i) {
		callbackInfo.channel_infos[i].channel = callbackInfo.buffer_infos[i].channelNum;
		callbackInfo.channel_infos[i].isInput = callbackInfo.buffer_infos[i].isInput;
		AsioIfFailedThrow(::ASIOGetChannelInfo(&callbackInfo.channel_infos[i]));
	}

	auto input_fomrat = output_format;

	ASIOChannelInfo channel_info{};
	channel_info.isInput = FALSE;
	AsioIfFailedThrow(::ASIOGetChannelInfo(&channel_info));

	mix_format_ = output_format;
	// ASIO output is DEINTERLEAVED format
	mix_format_.SetInterleavedFormat(InterleavedFormat::DEINTERLEAVED);

	switch (channel_info.type) {
	case ASIOSTInt16MSB:
		mix_format_.SetByteFormat(ByteFormat::SINT16);
		XAMP_LOG_INFO("Driver support format: ASIOSTInt16MSB");
		break;
	case ASIOSTInt16LSB:
		mix_format_.SetByteFormat(ByteFormat::SINT16);
		XAMP_LOG_INFO("Driver support format: ASIOSTInt16LSB");
		break;
	case ASIOSTInt24MSB:
		mix_format_.SetByteFormat(ByteFormat::SINT24);
		XAMP_LOG_INFO("Driver support format: ASIOSTInt24MSB");
		break;
	case ASIOSTInt24LSB:
		mix_format_.SetByteFormat(ByteFormat::SINT24);
		XAMP_LOG_INFO("Driver support format: ASIOSTInt24LSB");
		break;
	case ASIOSTFloat32MSB:
		mix_format_.SetByteFormat(ByteFormat::FLOAT32);
		XAMP_LOG_INFO("Driver support format: ASIOSTFloat32MSB");
		break;
	case ASIOSTFloat32LSB:
		mix_format_.SetByteFormat(ByteFormat::FLOAT32);
		XAMP_LOG_INFO("Driver support format: ASIOSTFloat32LSB");
		break;
	case ASIOSTInt32MSB:
		mix_format_.SetByteFormat(ByteFormat::SINT32);
		XAMP_LOG_INFO("Driver support format: ASIOSTInt32MSB");
		break;
	case ASIOSTInt32LSB:
		mix_format_.SetByteFormat(ByteFormat::SINT32);
		XAMP_LOG_INFO("Driver support format: ASIOSTInt32LSB");
		break;
		// DSD 8 bit data, 1 sample per byte. No Endianness required.
	case ASIOSTDSDInt8LSB1:
		mix_format_.SetByteFormat(ByteFormat::SINT8);
		XAMP_LOG_INFO("Driver support format: ASIOSTDSDInt8LSB1");
		break;
	case ASIOSTDSDInt8MSB1:
		mix_format_.SetByteFormat(ByteFormat::SINT8);
		XAMP_LOG_INFO("Driver support format: ASIOSTDSDInt8MSB1");
		break;
	case ASIOSTDSDInt8NER8:
		mix_format_.SetByteFormat(ByteFormat::SINT8);
		XAMP_LOG_INFO("Driver support format: ASIOSTDSDInt8NER8");
		break;
	default:
		throw ASIOException(Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT);
	}

	XAMP_LOG_INFO("Native DSD support: {}", IsSupportDsdFormat());

	device_buffer_vmlock_.UnLock();

	if (io_format_ == AsioIoFormat::IO_FORMAT_PCM) {
		size_t allocate_bytes = buffer_size_ * mix_format_.GetBytesPerSample() * mix_format_.GetChannels();
		callbackInfo.data_context = MakeConvert(input_fomrat, mix_format_, buffer_size_);
		buffer_bytes_ = buffer_size_ * (int64_t)mix_format_.GetBytesPerSample();
		buffer_ = MakeBuffer<int8_t>(allocate_bytes * buffer_size_);
		device_buffer_ = MakeBuffer<int8_t>(allocate_bytes * buffer_size_);
		buffer_vmlock_.Lock(buffer_.get(), allocate_bytes* buffer_size_);
		device_buffer_vmlock_.Lock(device_buffer_.get(), allocate_bytes * buffer_size_);
	}
	else {
		switch (channel_info.type) {
		case ASIOSTDSDInt8LSB1:
			if (sample_format_ != DsdFormat::DSD_INT8LSB) {
				throw ASIOException(Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT);
			}
			break;
		case ASIOSTDSDInt8MSB1:
			if (sample_format_ != DsdFormat::DSD_INT8MSB) {
				throw ASIOException(Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT);
			}
			break;
		case ASIOSTDSDInt8NER8:
			if (sample_format_ != DsdFormat::DSD_INT8NER8) {
				throw ASIOException(Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT);
			}
			break;
		}
		auto channel_buffer_size = buffer_size_ / 8;
		buffer_bytes_ = channel_buffer_size;
		int32_t allocate_bytes = buffer_size_;
		device_buffer_ = MakeBuffer<int8_t>(allocate_bytes);
		buffer_ = MakeBuffer<int8_t>(allocate_bytes);
		buffer_vmlock_.Lock(buffer_.get(), allocate_bytes);
		device_buffer_vmlock_.Lock(device_buffer_.get(), allocate_bytes);
		callbackInfo.data_context = MakeConvert(input_fomrat, mix_format_, channel_buffer_size);
	}

	long input_latency = 0;
	long output_latency = 0;
	AsioIfFailedThrow(::ASIOGetLatencies(&input_latency, &output_latency));
	XAMP_LOG_INFO("Ouput latency: {}ms", GetLatencyMs(output_latency, output_format.GetSampleRate()));
}

uint32_t AsioDevice::GetVolume() const {
	return volume_;
}

void AsioDevice::SetVolume(const uint32_t volume) const {
	volume_ = volume;
}

void AsioDevice::SetMute(const bool mute) const {
	if (mute) {
		volume_ = 0;
	}
}

void AsioDevice::OnBufferSwitch(long index) noexcept {
	if (callbackInfo.boost_priority) {
		callbackInfo.mmcss.BoostPriority();		
		SetCurrentThreadAffinity(1);
		callbackInfo.boost_priority = false;
	}

	const auto vol = volume_.load();
	if (callbackInfo.data_context.cache_volume != vol) {
		callbackInfo.data_context.volume_factor = LinearToLog(vol);
		callbackInfo.data_context.cache_volume = vol;
	}	

	auto cache_played_bytes = played_bytes_.load();
	cache_played_bytes += buffer_bytes_ * mix_format_.GetChannels();
	played_bytes_ = cache_played_bytes;

	if (!is_streaming_) {
		is_stop_streaming_ = true;
		condition_.notify_all();
		callbackInfo.mmcss.RevertPriority();
		return;
	}

	bool got_samples = false;

	if (io_format_ == AsioIoFormat::IO_FORMAT_PCM) {
		if (callback_->OnGetSamples(reinterpret_cast<float*>(buffer_.get()), buffer_size_, double(cache_played_bytes) / mix_format_.GetAvgBytesPerSec()) == 0) {
			switch (mix_format_.GetByteFormat()) {
			case ByteFormat::SINT16:
				DataConverter<InterleavedFormat::DEINTERLEAVED,
					InterleavedFormat::INTERLEAVED>::Convert(reinterpret_cast<int16_t*>(device_buffer_.get()),
						reinterpret_cast<const float*>(buffer_.get()),
						callbackInfo.data_context);
				break;
			case ByteFormat::SINT24:
				DataConverter<InterleavedFormat::DEINTERLEAVED,
					InterleavedFormat::INTERLEAVED>::Convert(reinterpret_cast<int24_t*>(device_buffer_.get()),
						reinterpret_cast<const float*>(buffer_.get()),
						callbackInfo.data_context);
				break;
			case ByteFormat::SINT32:
				DataConverter<InterleavedFormat::DEINTERLEAVED,
					InterleavedFormat::INTERLEAVED>::Convert(reinterpret_cast<int32_t*>(device_buffer_.get()),
						reinterpret_cast<const float*>(buffer_.get()),
						callbackInfo.data_context);
				break;
			default:
				break;
			}
			got_samples = true;
		}
	}
	else {
		const auto avg_byte_per_sec = mix_format_.GetAvgBytesPerSec() / 8;
		if (callback_->OnGetSamples(buffer_.get(), buffer_bytes_, double(played_bytes_) / avg_byte_per_sec) == 0) {
			DataConverter<InterleavedFormat::DEINTERLEAVED,
				InterleavedFormat::INTERLEAVED>::Convert(reinterpret_cast<int8_t*>(device_buffer_.get()),
					reinterpret_cast<const int8_t*>(buffer_.get()),
					callbackInfo.data_context);
			got_samples = true;
		}
	}

	if (got_samples) {
		for (size_t i = 0, j = 0; i < mix_format_.GetChannels(); ++i) {
			(void)FastMemcpy(callbackInfo.buffer_infos[i].buffers[index],
				&device_buffer_[j++ * buffer_bytes_],
				buffer_bytes_);
		}
		::ASIOOutputReady();
	}
}

void AsioDevice::OpenStream(const AudioFormat& output_format) {
	ASIODriverInfo asio_driver_info{};
	asio_driver_info.asioVersion = 2;
	asio_driver_info.sysRef = ::GetDesktopWindow();

	// 如果有播放過的話(callbackInfo.device != nullptr), ASIO每次必須要重新建立!
	ReOpen();

	if (device_id_.length() > sizeof(asio_driver_info.name) - 1) {
		(void)FastMemcpy(asio_driver_info.name, device_id_.c_str(), sizeof(asio_driver_info.name) - 1);
	}
	else {
		(void)FastMemcpy(asio_driver_info.name, device_id_.c_str(), device_id_.length());
	}

	AsioIfFailedThrow(::ASIOInit(&asio_driver_info));

	ASIOIoFormat asio_fomrmat{};
	if (io_format_ == AsioIoFormat::IO_FORMAT_DSD) {
		asio_fomrmat.FormatType = kASIODSDFormat;
	}
	else {
		asio_fomrmat.FormatType = kASIOPCMFormat;
	}

	try {
		AsioIfFailedThrow2(::ASIOFuture(kAsioSetIoFormat, &asio_fomrmat), ASE_SUCCESS);
	}
	catch (const Exception & e) {
		XAMP_LOG_DEBUG("ASIOFuture retun failure! {}", e.GetErrorMessage());
		// NOTE: DSD format must be support!
		if (output_format.GetFormat() == DataFormat::FORMAT_DSD) {
			throw;
		}
	}

	SetOutputSampleRate(output_format);
	CreateBuffers(output_format);

	played_bytes_ = 0;
	is_stopped_ = false;

	callbackInfo.boost_priority = true;
	callbackInfo.data_context.cache_volume = 0;
	callbackInfo.device = this;
}

void AsioDevice::SetOutputSampleRate(const AudioFormat& output_format) {
	auto error = ::ASIOSetSampleRate(static_cast<ASIOSampleRate>(output_format.GetSampleRate()));
	if (error == ASE_NotPresent) {
		throw DeviceUnSupportedFormatException(output_format);
	}
	AsioIfFailedThrow(error);
	XAMP_LOG_INFO("Set device samplerate: {}", output_format.GetSampleRate());

	clock_source_.resize(MAX_CLOCK_SOURCE_SIZE);

	auto num_clock_source = static_cast<long>(clock_source_.size());
	AsioIfFailedThrow(::ASIOGetClockSources(clock_source_.data(), &num_clock_source));

	auto is_current_source_set = false;
	if (num_clock_source > 0) {
		const auto itr = std::find_if(clock_source_.begin(), clock_source_.end(),
			[](const auto& source) {
				return source.isCurrentSource;
			});
		is_current_source_set = itr != clock_source_.end();
	}

	if (!is_current_source_set && num_clock_source > 1) {
		XAMP_LOG_INFO("Set device clock source: {}", clock_source_[0].index);
		AsioIfFailedThrow(::ASIOSetClockSource(clock_source_[0].index));
	}
}

void AsioDevice::SetAudioCallback(AudioCallback* callback) noexcept {
	callback_ = callback;
}

bool AsioDevice::IsStreamOpen() const noexcept {
	return !is_stopped_;
}

void AsioDevice::StopStream(bool wait_for_stop_stream) {
	if (!is_streaming_) {
		return;
	}

	is_streaming_ = false;
	is_stop_streaming_ = false;

	std::unique_lock<std::mutex> lock{ mutex_ };

	while (wait_for_stop_stream && !is_stop_streaming_) {
		condition_.wait(lock);
	}

	AsioIfFailedThrow(::ASIOStop());
}

void AsioDevice::CloseStream() {
	if (!is_stop_streaming_) {
		callbackInfo.drivers->removeCurrentDriver();
		return;
	}

	if (!is_removed_driver_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		AsioIfFailedThrow(::ASIOStop());

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		AsioIfFailedThrow(::ASIODisposeBuffers());

		if (callbackInfo.drivers != nullptr) {
			callbackInfo.drivers->removeCurrentDriver();
			callbackInfo.drivers.reset();
		}
		is_removed_driver_ = true;
	}
	callback_ = nullptr;
}

void AsioDevice::StartStream() {	
	AsioIfFailedThrow(::ASIOStart());
	is_streaming_ = true;
	is_stop_streaming_ = false;
}

bool AsioDevice::IsStreamRunning() const noexcept {
	return is_streaming_;
}

void AsioDevice::SetStreamTime(double stream_time) noexcept {
	if (io_format_ == AsioIoFormat::IO_FORMAT_PCM) {
		played_bytes_ = static_cast<int64_t>(stream_time * mix_format_.GetAvgBytesPerSec());
	}
	else {
		const auto avg_byte_per_sec = mix_format_.GetAvgBytesPerSec() / 8;
		played_bytes_ = stream_time * avg_byte_per_sec;
	}
}

double AsioDevice::GetStreamTime() const noexcept {
	return static_cast<double>(played_bytes_) / mix_format_.GetAvgBytesPerSec();
}

void AsioDevice::DisplayControlPanel() {
	AsioIfFailedThrow(::ASIOControlPanel());
}

InterleavedFormat AsioDevice::GetInterleavedFormat() const noexcept {
	return InterleavedFormat::DEINTERLEAVED;
}

ASIOTime* AsioDevice::OnBufferSwitchTimeInfoCallback(ASIOTime* timeInfo, long index, ASIOBool processNow) noexcept {
	return timeInfo;
}

void AsioDevice::OnBufferSwitchCallback(long index, ASIOBool processNow) {
	ASIOTime time_info{ 0 };
	if (::ASIOGetSamplePosition(&time_info.timeInfo.samplePosition, &time_info.timeInfo.systemTime) == ASE_OK) {
		time_info.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;
	}
	callbackInfo.device->OnBufferSwitchTimeInfoCallback(&time_info, index, processNow);
	callbackInfo.device->OnBufferSwitch(index);
}

long AsioDevice::OnAsioMessagesCallback(long selector, long value, void* message, double* opt) {
	long ret = 0;

	switch (selector) {
	case kAsioSelectorSupported:
		if (value == kAsioResetRequest
			|| value == kAsioEngineVersion
			|| value == kAsioResyncRequest
			|| value == kAsioLatenciesChanged
			// The following three were added for ASIO 2.0, you don't
			// necessarily have to support them.
			|| value == kAsioSupportsTimeInfo
			|| value == kAsioSupportsTimeCode
			|| value == kAsioSupportsInputMonitor)
			ret = 1L;
		break;
	case kAsioResetRequest:
		// Defer the task and perform the reset of the driver during the
		// next "safe" situation.  You cannot reset the driver right now,
		// as this code is called from the driver.  Reset the driver is
		// done by completely destruct is. I.e. ASIOStop(),
		// ASIODisposeBuffers(), Destruction Afterwards you initialize the
		// driver again.
		XAMP_LOG_INFO("Driver reset requested!!!");
		ret = 1L;
		break;
	case kAsioResyncRequest:
		// This informs the application that the driver encountered some
		// non-fatal data loss.  It is used for synchronization purposes
		// of different media.  Added mainly to work around the Win16Mutex
		// problems in Windows 95/98 with the Windows Multimedia system,
		// which could lose data because the Mutex was held too long by
		// another thread.  However a driver can issue it in other
		// situations, too.
		XAMP_LOG_INFO("Driver resync requested!!!");
		callbackInfo.is_xrun = true;
		ret = 1L;
		break;
	case kAsioLatenciesChanged:
		// This will inform the host application that the drivers were
		// latencies changed.  Beware, it this does not mean that the
		// buffer sizes have changed!  You might need to update internal
		// delay data.
		XAMP_LOG_INFO("Driver latency may have changed!!!");
		ret = 1L;
		break;
	case kAsioEngineVersion:
		// Return the supported ASIO version of the host application.  If
		// a host application does not implement this selector, ASIO 1.0
		// is assumed by the driver.
		ret = 2L;
		break;
	case kAsioSupportsTimeInfo:
		// Informs the driver whether the
		// asioCallbacks.bufferSwitchTimeInfo() callback is supported.
		// For compatibility with ASIO 1.0 drivers the host application
		// should always support the "old" bufferSwitch method, too.
		ret = 0;
		break;
	case kAsioSupportsTimeCode:
		// Informs the driver whether application is interested in time
		// code info.  If an application does not need to know about time
		// code, the driver has less work to do.
		ret = 0;
		break;
	}
	return ret;
}

void AsioDevice::OnSampleRateChangedCallback(ASIOSampleRate sampleRate) {
	// The ASIO documentation says that this usually only happens during
	// external sync.  Audio processing is not stopped by the driver,
	// actual sample rate might not have even changed, maybe only the
	// sample rate status of an AES/EBU or S/PDIF digital input at the
	// audio device.

	callbackInfo.device->StopStream();
	callbackInfo.device->callback_->OnError(SampleRateChangedException());
}

}

#endif

