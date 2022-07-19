#if ENABLE_ASIO

#include <asiodrivers.h>
#include <iasiodrv.h>

#include <base/volume.h>
#include <base/memory.h>
#include <base/align_ptr.h>
#include <base/str_utilts.h>
#include <base/dataconverter.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/singleton.h>
#include <base/platform.h>
#include <base/stopwatch.h>
#include <base/simd.h>

#ifdef XAMP_OS_WIN
#include <output_device/win32/mmcss.h>
#endif

#include <output_device/iaudiocallback.h>
#include <output_device/win32/asioexception.h>
#include <output_device/win32/asiodevice.h>

namespace xamp::output_device::win32 {

struct AsioDriver {
	bool is_xrun{ false };
	bool post_output{ false };
	AsioDevice* device{ nullptr };
	AlignPtr<AsioDrivers> drivers{};
	AudioConvertContext data_context{};
	ASIOCallbacks asio_callbacks{};
	Stopwatch buffer_switch_stopwatch;
	std::array<ASIOBufferInfo, AudioFormat::kMaxChannel> buffer_infos{};
	std::array<ASIOChannelInfo, AudioFormat::kMaxChannel> channel_infos{};

	void SetPostOutput() {
		post_output = ::ASIOOutputReady() == ASE_OK;
	}

	void Post() noexcept {
		if (post_output) {
			::ASIOOutputReady();
		}
	}
};

static XAMP_ALWAYS_INLINE long GetLatencyMs(long latency, long sampleRate) noexcept {
	return latency * 1000 / sampleRate;
}

static XAMP_ALWAYS_INLINE int64_t ASIO64toDouble(const ASIOSamples &a) noexcept {
#if NATIVE_INT64  
	return a;
#else
	constexpr double kTwoRaisedTo32 = 4294967296.;
	return (a).lo + (a).hi * kTwoRaisedTo32;
#endif
}

#define ASIODriver Singleton<AsioDriver>::GetInstance()

inline constexpr int32_t kClockSourceSize = 32;

AsioDevice::AsioDevice(std::string const & device_id)
	: is_removed_driver_(true)
	, is_stopped_(true)
	, is_streaming_(false)
	, is_stop_streaming_(false)
	, latency_(0)
	, io_format_(DsdIoFormat::IO_FORMAT_PCM)
	, sample_format_(DsdFormat::DSD_INT8MSB)
	, volume_(0)
	, buffer_size_(0)
	, buffer_bytes_(0)
	, played_bytes_(0)
	, device_id_(device_id)
	, clock_source_(kClockSourceSize)
	, callback_(nullptr)
	, process_(nullptr)
	, log_(Logger::GetInstance().GetLogger(kAsioDeviceLoggerName)) {
}

AsioDevice::~AsioDevice() {
	try {
		CloseStream();
	}
	catch (...) {
	}
	buffer_.reset();
	device_buffer_.reset();
}

bool AsioDevice::IsHardwareControlVolume() const {
	// NOTE :
	// Almost driver not support kAsioCanOutputGain, we always return true for software volume control.
	//return (io_format_ == DsdIoFormat::IO_FORMAT_PCM);
	ASIOIoFormat asio_fomrmat{};
	auto result = ::ASIOFuture(kAsioCanOutputGain, &asio_fomrmat);
	return (result == ASE_SUCCESS);
}

void AsioDevice::AbortStream() noexcept {
	is_stop_streaming_ = true;
}

bool AsioDevice::IsMuted() const {
	return volume_ == 0;
}

uint32_t AsioDevice::GetBufferSize() const noexcept {
	return buffer_size_ * format_.GetChannels();
}

void AsioDevice::SetSampleFormat(DsdFormat format) {
	sample_format_ = format;
}

DsdFormat AsioDevice::GetSampleFormat() const noexcept {
	return sample_format_;
}

void AsioDevice::SetIoFormat(DsdIoFormat format) {
	io_format_ = format;
}

DsdIoFormat AsioDevice::GetIoFormat() const {
	ASIOIoFormat asio_fomrmat{};
	AsioIfFailedThrow2(::ASIOFuture(kAsioGetIoFormat, &asio_fomrmat), ASE_SUCCESS);
	return (asio_fomrmat.FormatType == kASIOPCMFormat)
		? DsdIoFormat::IO_FORMAT_PCM : DsdIoFormat::IO_FORMAT_DSD;
}

bool AsioDevice::IsSupportDsdFormat() const {
	ASIOIoFormat asio_format{};
	asio_format.FormatType = kASIODSDFormat;
	auto error = ::ASIOFuture(kAsioCanDoIoFormat, &asio_format);
	return error == ASE_SUCCESS;
}

void AsioDevice::ResetDriver() {
	if (!ASIODriver.drivers) {
		return;
	}
	if (!ASIODriver.device) {
		return;
	}
	ASIODriver.device->ReOpen();
	ASIODriver.drivers.reset();
	XAMP_LOG_DEBUG("Remove ASIO driver");
}

void AsioDevice::RemoveCurrentDriver() {
	if (!is_removed_driver_ && ASIODriver.drivers != nullptr) {
		ASIODriver.drivers->removeCurrentDriver();
		is_removed_driver_ = true;
	}
}

void AsioDevice::ReOpen() {
	if (ASIODriver.drivers != nullptr) {
		is_removed_driver_ = false;
		return;
	}
	if (!ASIODriver.drivers) {
		ASIODriver.drivers = MakeAlign<AsioDrivers>();
	}	
	ASIODriver.drivers->removeCurrentDriver();
	if (!ASIODriver.drivers->loadDriver(const_cast<char*>(device_id_.c_str()))) {
		throw DeviceNotFoundException();
	}
	is_removed_driver_ = false;
}

std::tuple<int32_t, int32_t> AsioDevice::GetDeviceBufferSize() const {
	long min_size = 0;
	long max_size = 0;
	long prefer_size = 0;
	long granularity = 0;
	AsioIfFailedThrow(::ASIOGetBufferSize(&min_size, &max_size, &prefer_size, &granularity));

	XAMP_LOG_D(log_, "min_size:{} max_size:{} prefer_size:{} granularity:{}.",
		min_size,
		max_size,
		prefer_size,
		granularity);

	long buffer_size = prefer_size;

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

void AsioDevice::CreateBuffers(AudioFormat const & output_format) {
	const auto [prefer_size, buffer_size] = GetDeviceBufferSize();

	long num_channel = 0;
	for (auto& info : ASIODriver.buffer_infos) {
		info.isInput = ASIOFalse;
		info.channelNum = num_channel;
		info.buffers[0] = nullptr;
		info.buffers[1] = nullptr;
		++num_channel;
	}

	ASIODriver.asio_callbacks.bufferSwitch = OnBufferSwitchCallback;
	ASIODriver.asio_callbacks.sampleRateDidChange = OnSampleRateChangedCallback;
	ASIODriver.asio_callbacks.asioMessage = OnAsioMessagesCallback;
	ASIODriver.asio_callbacks.bufferSwitchTimeInfo = OnBufferSwitchTimeInfoCallback;
	ASIODriver.data_context.volume_factor = LinearToLog(volume_);

	const auto result = ::ASIOCreateBuffers(
		ASIODriver.buffer_infos.data(),
	    output_format.GetChannels(),
	    buffer_size,
	    &ASIODriver.asio_callbacks);
	if (result != ASE_OK) {
		AsioIfFailedThrow(::ASIOCreateBuffers(ASIODriver.buffer_infos.data(),
			output_format.GetChannels(),
			prefer_size,
			&ASIODriver.asio_callbacks));
		buffer_size_ = prefer_size;
	}
	else {
		buffer_size_ = buffer_size;
	}

	for (long i = 0; i < ASIODriver.buffer_infos.size(); ++i) {
		ASIODriver.channel_infos[i].channel =
			ASIODriver.buffer_infos[i].channelNum;
		ASIODriver.channel_infos[i].isInput =
			ASIODriver.buffer_infos[i].isInput;
		AsioIfFailedThrow(::ASIOGetChannelInfo(&ASIODriver.channel_infos[i]));
	}

	const auto in_format = output_format;

	ASIOChannelInfo channel_info{};
	channel_info.isInput = FALSE;
	AsioIfFailedThrow(::ASIOGetChannelInfo(&channel_info));

	format_ = output_format;
	// ASIO output always PLANAR format
	format_.SetPackedFormat(PackedFormat::PLANAR);

	switch (channel_info.type) {
	case ASIOSTInt16MSB:
		format_.SetByteFormat(ByteFormat::SINT16);
		XAMP_LOG_D(log_, "Driver support format: ASIOSTInt16MSB.");
		break;
	case ASIOSTInt16LSB:
		format_.SetByteFormat(ByteFormat::SINT16);
		XAMP_LOG_D(log_, "Driver support format: ASIOSTInt16LSB.");
		break;
	case ASIOSTInt24MSB:
		format_.SetByteFormat(ByteFormat::SINT24);
		XAMP_LOG_D(log_, "Driver support format: ASIOSTInt24MSB.");
		break;
	case ASIOSTInt24LSB:
		format_.SetByteFormat(ByteFormat::SINT24);
		XAMP_LOG_D(log_, "Driver support format: ASIOSTInt24LSB.");
		break;
	case ASIOSTFloat32MSB:
		format_.SetByteFormat(ByteFormat::FLOAT32);
		XAMP_LOG_D(log_, "Driver support format: ASIOSTFloat32MSB.");
		break;
	case ASIOSTFloat32LSB:
		format_.SetByteFormat(ByteFormat::FLOAT32);
		XAMP_LOG_D(log_, "Driver support format: ASIOSTFloat32LSB.");
		break;
	case ASIOSTInt32MSB:
		format_.SetByteFormat(ByteFormat::SINT32);
		XAMP_LOG_D(log_, "Driver support format: ASIOSTInt32MSB.");
		break;
	case ASIOSTInt32LSB:
		format_.SetByteFormat(ByteFormat::SINT32);
		XAMP_LOG_D(log_, "Driver support format: ASIOSTInt32LSB.");
		break;
		// DSD 8 bit data, 1 sample per byte. No Endianness required.
	case ASIOSTDSDInt8LSB1:
		format_.SetByteFormat(ByteFormat::SINT8);
		XAMP_LOG_D(log_, "Driver support format: ASIOSTDSDInt8LSB1.");
		break;
	case ASIOSTDSDInt8MSB1:
		format_.SetByteFormat(ByteFormat::SINT8);
		XAMP_LOG_D(log_, "Driver support format: ASIOSTDSDInt8MSB1.");
		break;
	case ASIOSTDSDInt8NER8:
		format_.SetByteFormat(ByteFormat::SINT8);
		XAMP_LOG_D(log_, "Driver support format: ASIOSTDSDInt8NER8.");
		break;
	default:
		throw ASIOException(Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT);
	}

	XAMP_LOG_D(log_, "Native DSD support: {}.", IsSupportDsdFormat());

	if (io_format_ == DsdIoFormat::IO_FORMAT_PCM) {
		const auto allocate_bytes = buffer_size_ * format_.GetBytesPerSample() * format_.GetChannels();
		ASIODriver.data_context = MakeConvert(in_format, format_, buffer_size_);
		buffer_bytes_ = buffer_size_ * static_cast<int64_t>(format_.GetBytesPerSample());
		const auto alloc_size = allocate_bytes;
		if (buffer_.GetSize() < alloc_size) {
			buffer_.reset();
			buffer_ = MakeBuffer<int8_t>(alloc_size);
		}
		if (device_buffer_.GetSize() < alloc_size) {
			device_buffer_.reset();
			device_buffer_ = MakeBuffer<int8_t>(alloc_size);
		}		
	}
	else {
		switch (channel_info.type) {
		case ASIOSTDSDInt8LSB1:
			if (sample_format_ != DsdFormat::DSD_INT8LSB) {
				throw NotSupportFormatException();
			}
			break;
		case ASIOSTDSDInt8MSB1:
			if (sample_format_ != DsdFormat::DSD_INT8MSB) {
				throw NotSupportFormatException();
			}
			break;
		case ASIOSTDSDInt8NER8:
			if (sample_format_ != DsdFormat::DSD_INT8NER8) {
				throw NotSupportFormatException();
			}
			break;
		default:
			throw NotSupportFormatException();
		}
		const auto channel_buffer_size = buffer_size_ / 8;
		buffer_bytes_ = channel_buffer_size;
		const auto allocate_bytes = buffer_size_;
		if (device_buffer_.GetSize() < allocate_bytes) {
			device_buffer_.reset();
			device_buffer_ = MakeBuffer<int8_t>(allocate_bytes);
		}		
		if (buffer_.GetSize() < allocate_bytes) {
			buffer_.reset();
			buffer_ = MakeBuffer<int8_t>(allocate_bytes);
		}		
		ASIODriver.data_context = MakeConvert(in_format, format_, channel_buffer_size);
	}

	long input_latency = 0;
	long output_latency = 0;
	AsioIfFailedThrow(::ASIOGetLatencies(&input_latency, &output_latency));
	latency_ = GetLatencyMs(output_latency, output_format.GetSampleRate());
	XAMP_LOG_D(log_, "Buffer size :{} ", String::FormatBytes(buffer_.GetByteSize()));
	XAMP_LOG_D(log_, "Output latency: {} msec.", latency_);
	ASIODriver.SetPostOutput();
	XAMP_LOG_D(log_, "Drvier support post output: {}", ASIODriver.post_output);
}

uint32_t AsioDevice::GetVolume() const {
	return volume_;
}

void AsioDevice::SetVolume(uint32_t volume) const {
	volume_ = std::clamp(volume, static_cast<uint32_t>(0), static_cast<uint32_t>(99));
	XAMP_LOG_D(log_, "Current volume: {}({} db)", GetVolume(), VolumeToDb(volume_));
}

void AsioDevice::SetMute(bool mute) const {
	if (mute) {
		volume_ = 0;
	}
}

void AsioDevice::FillSlientData() noexcept {
	for (size_t i = 0, j = 0; i < format_.GetChannels(); ++i) {
		MemorySet(ASIODriver.buffer_infos[i].buffers[0],
			0,
			buffer_bytes_);
		MemorySet(ASIODriver.buffer_infos[i].buffers[1],
			0,
			buffer_bytes_);
	}
}

bool AsioDevice::GetPCMSamples(long index, double sample_time, size_t& num_filled_frame) noexcept {
	const auto vol = volume_.load();
	if (ASIODriver.data_context.cache_volume != vol) {
		ASIODriver.data_context.volume_factor = LinearToLog(vol);
		ASIODriver.data_context.cache_volume = vol;
	}

	// PCM mode input float to output format.
	const auto stream_time = static_cast<double>(played_bytes_) / format_.GetAvgBytesPerSec();
	buffer_.Fill(0);

	XAMP_LIKELY(callback_->OnGetSamples(buffer_.Get(), buffer_size_, num_filled_frame, stream_time, sample_time) == DataCallbackResult::CONTINUE) {
		const auto in = reinterpret_cast<const float*>(buffer_.Get());
		InterleaveToPlanar<float, int32_t>::Convert(
			in,
			static_cast<int32_t*>(ASIODriver.buffer_infos[0].buffers[index]),
			static_cast<int32_t*>(ASIODriver.buffer_infos[1].buffers[index]),
			buffer_size_ * format_.GetChannels(),
			ASIODriver.data_context.volume_factor);
		return true;
	} else {
		return false;
	}
}

bool AsioDevice::GetDSDSamples(long index, double sample_time, size_t& num_filled_frame) noexcept {
	// DSD mode input output same format (int8_t).
	const auto avg_byte_per_sec = format_.GetAvgBytesPerSec() / 8;
	const auto stream_time = static_cast<double>(played_bytes_) / avg_byte_per_sec;

	XAMP_LIKELY(callback_->OnGetSamples(buffer_.Get(), buffer_bytes_, num_filled_frame, stream_time, sample_time) == DataCallbackResult::CONTINUE) {
		DataConverter<PackedFormat::PLANAR,
			PackedFormat::INTERLEAVED>::Convert(
				device_buffer_.Get(),
				buffer_.Get(),
				ASIODriver.data_context);
		for (size_t i = 0, j = 0; i < format_.GetChannels(); ++i) {
			MemoryCopy(ASIODriver.buffer_infos[i].buffers[index],
				&device_buffer_[j++ * buffer_bytes_],
				buffer_bytes_);
		}
		return true;
	} else {
		return false;
	}
}

void AsioDevice::OnBufferSwitch(long index, double sample_time) noexcept {
	if (!is_streaming_) {
		XAMP_LOG_D(log_, "Stream was stopped!1");
		// 填充靜音的資料並通知結束.
		FillSlientData();
		is_stop_streaming_ = true;
		condition_.notify_all();
		ASIODriver.Post();
		return;
	}

	if (is_stop_streaming_) {
		XAMP_LOG_D(log_, "Stream was stopped!2");
	}
	
	auto cache_played_bytes = played_bytes_.load();
	if (cache_played_bytes == 0) {
		mmcss_.BoostPriority();
	}
	
	cache_played_bytes += buffer_bytes_ * format_.GetChannels();
	played_bytes_ = cache_played_bytes;	

	size_t num_filled_frame = 0;
	const auto got_samples = (*this.*process_)(index, sample_time, num_filled_frame);
	
	if (got_samples) {
		ASIODriver.Post();
	} else {
		FillSlientData();
	}
}

void AsioDevice::OpenStream(AudioFormat const & output_format) {
	ASIODriverInfo asio_driver_info{};
	asio_driver_info.asioVersion = 2;
	asio_driver_info.sysRef = ::GetDesktopWindow();

	// 如果有播放過的話(callbackInfo.device != nullptr), ASIO每次必須要重新建立!
	ReOpen();

	auto name_len = (std::min)(sizeof(asio_driver_info.name) - 1, device_id_.length());
	MemoryCopy(asio_driver_info.name, device_id_.c_str(), name_len);

	auto result = ::ASIOInit(&asio_driver_info);
	if (result == ASE_NotPresent) {
		ASIODriver.drivers.reset();
		throw ASIOException(Errors::XAMP_ERROR_DEVICE_NOT_FOUND);
	}
	else if (result != ASE_OK) {
		throw ASIOException(result);
	}	

	ASIOIoFormat asio_fomrmat{};
	if (io_format_ == DsdIoFormat::IO_FORMAT_DSD) {
		asio_fomrmat.FormatType = kASIODSDFormat;
	}
	else {
		asio_fomrmat.FormatType = kASIOPCMFormat;
	}

	try {
		AsioIfFailedThrow2(::ASIOFuture(kAsioSetIoFormat, &asio_fomrmat), ASE_SUCCESS);
	}
	catch (const Exception & e) {
		XAMP_LOG_D(log_, "ASIOFuture retun failure. {}", e.GetErrorMessage());
		// NOTE: DSD format must be support!
		if (output_format.GetFormat() == DataFormat::FORMAT_DSD) {
			throw;
		}
	}

	SetOutputSampleRate(output_format);
	CreateBuffers(output_format);

	played_bytes_ = 0;
	is_stopped_ = false;
	ASIODriver.data_context.cache_volume = 0;
	ASIODriver.device = this;
}

void AsioDevice::SetOutputSampleRate(AudioFormat const & output_format) {
	auto error = ::ASIOSetSampleRate(output_format.GetSampleRate());
	if (error == ASE_NotPresent) {
		throw DeviceUnSupportedFormatException(output_format);
	}
	AsioIfFailedThrow(error);
	XAMP_LOG_D(log_, "Set device samplerate: {}.", output_format.GetSampleRate());

	clock_source_.resize(kClockSourceSize);

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
		XAMP_LOG_D(log_, "Set device clock source: {}.", clock_source_[0].name);
		AsioIfFailedThrow(::ASIOSetClockSource(clock_source_[0].index));
	}
}

void AsioDevice::SetAudioCallback(IAudioCallback* callback) noexcept {
	callback_ = callback;
}

bool AsioDevice::IsStreamOpen() const noexcept {
	return !is_stopped_;
}

void AsioDevice::StopStream(bool wait_for_stop_stream) {
	if (!is_streaming_) {
		return;
	}

	XAMP_LOG_D(log_, "StopStream");

	is_streaming_ = false;
	std::unique_lock lock{ mutex_ };

	while (wait_for_stop_stream && !is_stop_streaming_) {
		condition_.wait(lock);
	}

	// todo: Workaround!
	// 等待10ms這段期間OnBufferSwitch會填充靜音的資料.	
	MSleep(std::chrono::milliseconds(10));
	AsioIfFailedThrow(::ASIOStop());
}

void AsioDevice::CloseStream() {
	XAMP_LOG_D(log_, "CloseStream");

	if (!is_removed_driver_) {
		AsioIfFailedThrow(::ASIOStop());		
		AsioIfFailedThrow(::ASIODisposeBuffers());
		is_removed_driver_ = true;
	}
	callback_ = nullptr;	
}

void AsioDevice::StartStream() {
	if (io_format_ == DsdIoFormat::IO_FORMAT_PCM) {
		process_ = &AsioDevice::GetPCMSamples;
	}
	else {
		process_ = &AsioDevice::GetDSDSamples;
	}

	mmcss_.RevertPriority();
	is_streaming_ = true;
	is_stop_streaming_ = false;
	ASIODriver.buffer_switch_stopwatch.Reset();
	AsioIfFailedThrow(::ASIOStart());
}

bool AsioDevice::IsStreamRunning() const noexcept {
	return is_streaming_;
}

void AsioDevice::SetStreamTime(double stream_time) noexcept {
	if (io_format_ == DsdIoFormat::IO_FORMAT_PCM) {
		played_bytes_ = static_cast<int64_t>(stream_time * format_.GetAvgBytesPerSec());
	}
	else {
		const auto avg_byte_per_sec = format_.GetAvgBytesPerSec() / 8;
		played_bytes_ = stream_time * avg_byte_per_sec;
	}
}

double AsioDevice::GetStreamTime() const noexcept {
	if (io_format_ == DsdIoFormat::IO_FORMAT_PCM) {
		return static_cast<double>(played_bytes_) / format_.GetAvgBytesPerSec();
	} else {
		const auto avg_byte_per_sec = format_.GetAvgBytesPerSec() / 8;
		return static_cast<double>(played_bytes_) / avg_byte_per_sec;
	}
}

PackedFormat AsioDevice::GetPackedFormat() const noexcept {
	return PackedFormat::PLANAR;
}

ASIOTime* AsioDevice::OnBufferSwitchTimeInfoCallback(ASIOTime* timeInfo, long index, ASIOBool processNow) noexcept {
	return timeInfo;
}

void AsioDevice::OnBufferSwitchCallback(long index, ASIOBool processNow) {
	ASIOTime time_info{ 0 };
	if (::ASIOGetSamplePosition(&time_info.timeInfo.samplePosition, &time_info.timeInfo.systemTime) == ASE_OK) {
		time_info.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;
	}
	
	ASIODriver.device->OnBufferSwitchTimeInfoCallback(&time_info, index, processNow);
	double sample_time = 0;
	if (time_info.timeInfo.flags & kSamplePositionValid) {
		sample_time = ASIO64toDouble(time_info.timeInfo.samplePosition)
		/ ASIODriver.device->format_.GetSampleRate();
	}

	auto elapsed = ASIODriver.buffer_switch_stopwatch.Elapsed<std::chrono::milliseconds>().count();
	if (elapsed > ASIODriver.device->latency_) {
		XAMP_LOG_D(ASIODriver.device->log_, "Wait event timeout! {}ms", elapsed);
	}

	ASIODriver.device->OnBufferSwitch(index, sample_time);
	ASIODriver.buffer_switch_stopwatch.Reset();
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
		ASIODriver.device->AbortStream();		
		ASIODriver.device->condition_.notify_one();
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
		ASIODriver.is_xrun = true;
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

	ASIODriver.device->StopStream();
	ASIODriver.device->callback_->OnError(SampleRateChangedException());
}

}

#endif

