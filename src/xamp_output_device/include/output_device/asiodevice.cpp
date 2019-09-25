#include <asiodrivers.h>
#include <iasiodrv.h>

#include <base/memory.h>
#include <base/align_ptr.h>
#include <base/str_utilts.h>
#include <base/dataconverter.h>
#include <base/logger.h>

#include <output_device/asioexception.h>
#include <output_device/asiodevice.h>

namespace xamp::output_device {

using namespace base;

struct AsioCallbackInfo {
	AsioCallbackInfo()
		: asioXRun(false)
		, device(nullptr) {
	}

    ~AsioCallbackInfo() {
		// Some asio driver always in system tray.
        // drivers.removeCurrentDriver();
    }

	bool asioXRun;
	AlignPtr<AsioDrivers> drivers;
	ConvertContext data_context;
	ASIOCallbacks asio_callbacks;
	AsioDevice* device;
	std::vector<ASIOBufferInfo> buffer_infos;
	std::vector<ASIOChannelInfo> channel_infos;
} callbackInfo;

#define GetLatencyMs(latency, sampleRate) (long((latency * 1000)/ sampleRate))

static const int32_t MAX_CLOCK_SOURCE_SIZE = 32;

AsioDevice::AsioDevice(const std::string& device_id)
	: is_removed_driver_(true)
	, is_stopped_(true)
	, is_streaming_(false)
	, is_stop_streaming_(false)
	, sample_format_(DSDSampleFormat::DSD_INT8MSB)
	, io_format_(AsioIoFormat::IO_FORMAT_PCM)
	, volume_(0)
	, buffer_size_(0)
	, buffer_bytes_(0)
	, played_bytes_(0)
	, device_id_(device_id)
	, clock_source_(MAX_CLOCK_SOURCE_SIZE)
	, callback_(nullptr) {
}

bool AsioDevice::IsMuted() const {
	return volume_ == 0;
}

int32_t AsioDevice::GetBufferSize() const {
	return buffer_size_ * mix_format_.GetChannels();
}

void AsioDevice::SetSampleFormat(DSDSampleFormat format) {
	sample_format_ = format;
}

DSDSampleFormat AsioDevice::GetSampleFormat() const {
	return sample_format_;
}

void AsioDevice::SetIoFormat(AsioIoFormat format) {
	io_format_ = format;
}

AsioIoFormat AsioDevice::GetIoFormat() const {
	ASIOIoFormat asio_fomrmat{};
	ASIO_IF_FAILED_THROW2(ASIOFuture(kAsioGetIoFormat, &asio_fomrmat), ASE_SUCCESS);
	return (asio_fomrmat.FormatType == kASIOPCMFormat)
		? AsioIoFormat::IO_FORMAT_PCM : AsioIoFormat::IO_FORMAT_DSD;
}

bool AsioDevice::IsSupportDSDFormat() const {
	ASIOIoFormat asio_format{};
	asio_format.FormatType = kASIODSDFormat;
	auto error = ASIOFuture(kAsioCanDoIoFormat, &asio_format);
	return error == ASE_SUCCESS;
}

void AsioDevice::ReOpen() {
	callbackInfo.drivers->removeCurrentDriver();
	if (!callbackInfo.drivers->loadDriver(const_cast<char*>(device_id_.c_str()))) {
		throw ASIOException(XAMP_ERROR_DEVICE_NOT_FOUND);
	}
	is_removed_driver_ = false;
}

void AsioDevice::CreateBuffers(const AudioFormat& output_format) {
	long min_size = 0;
	long max_size = 0;
	long prefer_size = 0;
	long granularity = 0;
	ASIO_IF_FAILED_THROW(ASIOGetBufferSize(&min_size, &max_size, &prefer_size, &granularity));

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

	callbackInfo.buffer_infos.resize(output_format.GetChannels());
	callbackInfo.channel_infos.resize(output_format.GetChannels());

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

	auto result = ASIOCreateBuffers(callbackInfo.buffer_infos.data(),
		output_format.GetChannels(),
		buffer_size,
		&callbackInfo.asio_callbacks);
	if (result != ASE_OK) {
		ASIO_IF_FAILED_THROW(ASIOCreateBuffers(callbackInfo.buffer_infos.data(),
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
		ASIO_IF_FAILED_THROW(ASIOGetChannelInfo(&callbackInfo.channel_infos[i]));
	}

	auto input_fomrat = output_format;

	ASIOChannelInfo channel_info{};
	channel_info.isInput = FALSE;
	ASIO_IF_FAILED_THROW(ASIOGetChannelInfo(&channel_info));

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
		throw ASIOException(XAMP_ERROR_NOT_SUPPORT_FORMAT);
	}

	XAMP_LOG_INFO("Native DSD support: {}", IsSupportDSDFormat());

	if (io_format_ == AsioIoFormat::IO_FORMAT_PCM) {
		int32_t allocate_bytes = buffer_size_ * mix_format_.GetBytesPerSample() * mix_format_.GetChannels();
		callbackInfo.data_context = MakeConvert(input_fomrat, mix_format_, buffer_size_);
		buffer_bytes_ = buffer_size_ * mix_format_.GetBytesPerSample();
		buffer_ = MakeBuffer<int8_t>(allocate_bytes * buffer_size_);
		device_buffer_ = MakeBuffer<int8_t>(allocate_bytes * buffer_size_);
		XAMP_LOG_INFO("buffer_size:{}, buffer_bytes:{}, allocate_bytes:{}",
			ByteSizeToString(buffer_size_),
			ByteSizeToString(buffer_bytes_),
			ByteSizeToString(allocate_bytes * buffer_size_));
	}
	else {
		switch (channel_info.type) {
		case ASIOSTDSDInt8LSB1:
			if (sample_format_ != DSDSampleFormat::DSD_INT8LSB) {
				throw ASIOException(XAMP_ERROR_NOT_SUPPORT_FORMAT);
			}
			break;
		case ASIOSTDSDInt8MSB1:
			if (sample_format_ != DSDSampleFormat::DSD_INT8MSB) {
				throw ASIOException(XAMP_ERROR_NOT_SUPPORT_FORMAT);
			}
			break;
		case ASIOSTDSDInt8NER8:
			if (sample_format_ != DSDSampleFormat::DSD_INT8NER8) {
				throw ASIOException(XAMP_ERROR_NOT_SUPPORT_FORMAT);
			}
			break;
		}
		auto channel_buffer_size = buffer_size_ / 8;
		buffer_bytes_ = channel_buffer_size;
		int32_t allocate_bytes = buffer_size_;
		XAMP_LOG_INFO("io format:{}, sample format:{}, buffer_size:{}, buffer_bytes:{}, allocate_bytes:{}",
			io_format_,
			sample_format_,
			ByteSizeToString(buffer_size_),
			ByteSizeToString(buffer_bytes_),
			ByteSizeToString(allocate_bytes));
		device_buffer_ = MakeBuffer<int8_t>(allocate_bytes);
		buffer_ = MakeBuffer<int8_t>(allocate_bytes);
		callbackInfo.data_context = MakeConvert(input_fomrat, mix_format_, channel_buffer_size);
	}

	long input_latency = 0;
	long output_latency = 0;
	ASIO_IF_FAILED_THROW(ASIOGetLatencies(&input_latency, &output_latency));
	XAMP_LOG_INFO("Input latency: {}ms Ouput latency: {}ms",
		GetLatencyMs(input_latency, output_format.GetSampleRate()),
		GetLatencyMs(output_latency, output_format.GetSampleRate()));
}

void AsioDevice::OnBufferSwitch(long index) {
	played_bytes_ += buffer_bytes_ * mix_format_.GetChannels();

	if (!is_streaming_) {
		is_stop_streaming_ = true;
		condition_.notify_all();
		return;
	}

	bool got_samples = false;

	if (io_format_ == AsioIoFormat::IO_FORMAT_PCM) {
		if ((*callback_)(reinterpret_cast<float*>(buffer_.get()), buffer_size_, double(played_bytes_) / mix_format_.GetAvgBytesPerSec()) == 0) {
			switch (mix_format_.GetByteFormat()) {
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
		if ((*callback_)(buffer_.get(), buffer_bytes_, played_bytes_ / avg_byte_per_sec) == 0) {
			DataConverter<InterleavedFormat::DEINTERLEAVED,
				InterleavedFormat::INTERLEAVED>::Convert(reinterpret_cast<int8_t*>(device_buffer_.get()),
					reinterpret_cast<const int8_t*>(buffer_.get()),
					callbackInfo.data_context);
			got_samples = true;
		}
	}

	if (got_samples) {
		for (int32_t i = 0, j = 0; i < mix_format_.GetChannels(); i++) {
			FastMemcpy(callbackInfo.buffer_infos[i].buffers[index], &device_buffer_[j++ * buffer_bytes_], buffer_bytes_);
		}
		ASIOOutputReady();
	}
}

void AsioDevice::OpenStream(const AudioFormat& output_format) {
	ASIODriverInfo asio_driver_info{};
	asio_driver_info.asioVersion = 2;
	asio_driver_info.sysRef = GetDesktopWindow();

	// 如果有播放過的話(callbackInfo.device != nullptr), ASIO每次必須要重新建立!
	if (callbackInfo.device != nullptr) {
		callbackInfo.drivers = MakeAlign<AsioDrivers>();
		ReOpen();
	}

	if (device_id_.length() > sizeof(asio_driver_info.name) - 1) {
		FastMemcpy(asio_driver_info.name, device_id_.c_str(), sizeof(asio_driver_info.name) - 1);
	}
	else {
		FastMemcpy(asio_driver_info.name, device_id_.c_str(), device_id_.length());
	}

	ASIO_IF_FAILED_THROW(ASIOInit(&asio_driver_info));

	ASIOIoFormat asio_fomrmat{};
	if (io_format_ == AsioIoFormat::IO_FORMAT_DSD) {
		asio_fomrmat.FormatType = kASIODSDFormat;
	}
	else {
		asio_fomrmat.FormatType = kASIOPCMFormat;
	}
	ASIO_IF_FAILED_THROW2(ASIOFuture(kAsioSetIoFormat, &asio_fomrmat), ASE_SUCCESS);

	SetOutputSampleRate(output_format);
	CreateBuffers(output_format);

	played_bytes_ = 0;
	is_stopped_ = false;
}

void AsioDevice::SetOutputSampleRate(const AudioFormat& output_format) {
	ASIO_IF_FAILED_THROW(ASIOSetSampleRate(static_cast<ASIOSampleRate>(output_format.GetSampleRate())));
	XAMP_LOG_INFO("Set device samplerate: {}", output_format.GetSampleRate());

	clock_source_.resize(MAX_CLOCK_SOURCE_SIZE);

	long num_clock_source = static_cast<long>(clock_source_.size());
	ASIO_IF_FAILED_THROW(ASIOGetClockSources(clock_source_.data(), &num_clock_source));

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
		ASIO_IF_FAILED_THROW(ASIOSetClockSource(clock_source_[0].index));
	}
}

void AsioDevice::SetAudioCallback(AudioCallback* callback) {
	callback_ = callback;
}

bool AsioDevice::IsStreamOpen() const {
	return !is_stopped_;
}

void AsioDevice::StopStream() {
	if (!is_streaming_) {
		return;
	}

	is_streaming_ = false;
	is_stop_streaming_ = false;

	std::unique_lock<std::mutex> lock{ mutex_ };

	while (!is_stop_streaming_) {
		condition_.wait(lock);
	}

	ASIO_IF_FAILED_THROW(ASIOStop());
}

void AsioDevice::CloseStream() {
	if (!is_stop_streaming_) {
		return;
	}

	if (!is_removed_driver_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		ASIO_IF_FAILED_THROW(ASIOStop());

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		ASIO_IF_FAILED_THROW(ASIODisposeBuffers());

		if (callbackInfo.drivers != nullptr) {
			callbackInfo.drivers->removeCurrentDriver();
			callbackInfo.drivers.reset();
		}
		is_removed_driver_ = true;
	}
	callback_ = nullptr;
}

void AsioDevice::StartStream() {
	callbackInfo.device = this;
	ASIO_IF_FAILED_THROW(ASIOStart());
	is_streaming_ = true;
	is_stop_streaming_ = false;
}

bool AsioDevice::IsStreamRunning() const {
	return is_streaming_;
}

void AsioDevice::SetStreamTime(const double stream_time) {
	played_bytes_ = static_cast<int32_t>((stream_time + 0.5) * mix_format_.GetAvgBytesPerSec());
}

double AsioDevice::GetStreamTime() const {
	return double(played_bytes_) / mix_format_.GetAvgBytesPerSec();
}

int32_t AsioDevice::GetVolume() const {
	return int32_t(volume_ * 100.0);
}

void AsioDevice::SetVolume(const int32_t volume) const {
	volume_ = volume / 100.0;
}

void AsioDevice::SetMute(const bool mute) const {
	if (mute) {
		volume_ = 0.0;
	}
}

void AsioDevice::DisplayControlPanel() {
	ASIO_IF_FAILED_THROW(ASIOControlPanel());
}

InterleavedFormat AsioDevice::GetInterleavedFormat() const {
	return InterleavedFormat::DEINTERLEAVED;
}

ASIOTime* AsioDevice::OnBufferSwitchTimeInfoCallback(ASIOTime* timeInfo, long index, ASIOBool processNow) {
	return timeInfo;
}

void AsioDevice::OnBufferSwitchCallback(long index, ASIOBool processNow) {
	ASIOTime time_info{ 0 };
	if (ASIOGetSamplePosition(&time_info.timeInfo.samplePosition, &time_info.timeInfo.systemTime) == ASE_OK) {
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
		callbackInfo.asioXRun = true;
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
}


}
