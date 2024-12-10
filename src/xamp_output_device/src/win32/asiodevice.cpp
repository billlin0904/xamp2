#include <base/base.h>

#if XAMP_OS_WIN

#include <asiodrivers.h>
#include <iasiodrv.h>

#include <base/volume.h>
#include <base/memory.h>
#include <base/str_utilts.h>
#include <base/dataconverter.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/singleton.h>
#include <base/platform.h>
#include <base/stopwatch.h>
#include <base/simd.h>

#include <output_device/win32/mmcss.h>
#include <output_device/win32/asiodevice.h>
#include <output_device/iaudiocallback.h>
#include <output_device/win32/asioexception.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

namespace {
	/*
	* ASIODriverContext is the asio driver.
	*
	*/
	struct ASIODriverContext {
		bool is_xrun{ false };
		bool post_output{ false };
		bool mmcss_registered{ false };
		AsioDevice* device{ nullptr };
		ScopedPtr<AsioDrivers> drivers{};
		AudioConvertContext data_context{};
		ASIOCallbacks asio_callbacks{};
		Stopwatch buffer_switch_stopwatch;
		Mmcss mmcss;
		std::string driver_name;
		std::array<ASIOBufferInfo, AudioFormat::kMaxChannel> buffer_infos{};
		std::array<ASIOChannelInfo, AudioFormat::kMaxChannel> channel_infos{};

		std::string GetDriverName() const {
			char driver_name[MAX_PATH]{};
			drivers->getCurrentDriverName(driver_name);
			return driver_name;
		}

		void SetPostOutput() {
			post_output = ::ASIOOutputReady() == ASE_OK;
		}

		void Post() noexcept {
			if (post_output) {
				::ASIOOutputReady();
			}
		}
	};

	/*
	* GetLatencyMs converts ASIO sample to ms.
	*
	* @param latency: ASIO sample
	* @param sampleRate: sample rate
	* @return ms
	*/
	XAMP_ALWAYS_INLINE long GetLatencyMs(long latency, long sampleRate) {
		return latency * 1000 / sampleRate;
	}

	/*
	* ASIO64toDouble converts ASIOSamples to double.
	*
	* @param a: ASIOTimeStamp
	* @return double
	*/
	XAMP_ALWAYS_INLINE double ASIO64toDouble(const ASIOTimeStamp& a) {
#if NATIVE_INT64  
		return a;
#else
		constexpr double kTwoRaisedTo32 = 4294967296.;
		return (a).lo + (a).hi * kTwoRaisedTo32;
#endif
	}

	/*
	* ASIO64toDouble converts ASIOSamples to double.
	*
	* @param a: ASIOSamples
	* @return double
	*/
	XAMP_ALWAYS_INLINE double ASIO64toDouble(const ASIOSamples& a) {
#if NATIVE_INT64  
		return a;
#else
		constexpr double kTwoRaisedTo32 = 4294967296.;
		return (a).lo + (a).hi * kTwoRaisedTo32;
#endif
	}

	ASIODriverContext the_driver_context;
}

inline constexpr int32_t kClockSourceSize = 32;

XAMP_DECLARE_LOG_NAME(AsioDevice);

AsioDevice::AsioDevice(const  std::string & device_id)
	: is_hardware_control_volume_(true)
	, is_removed_driver_(true)
	, is_stopped_(true)
	, is_streaming_(false)
	, is_stop_streaming_(false)
	, latency_(0)
	, io_format_(DsdIoFormat::IO_FORMAT_PCM)
	, sample_format_(DsdFormat::DSD_INT8MSB)
	, volume_(0)
	, buffer_size_(0)
	, buffer_bytes_(0)
	, output_bytes_(0)
	, clock_source_(kClockSourceSize)
	, callback_(nullptr)
	, get_samples_(nullptr)
	, logger_(XampLoggerFactory.GetLogger(kAsioDeviceLoggerName))
	, device_id_(device_id) {
}

AsioDevice::~AsioDevice() {
	try {
		CloseStream();
	}
	catch (...) {
	}
	buffer_.reset();
	device_buffer_.reset();
	the_driver_context.drivers.reset();
}

bool AsioDevice::IsHardwareControlVolume() const {	
	if (is_streaming_) {
		return true;
	}

	return is_hardware_control_volume_;
}

void AsioDevice::AbortStream() noexcept {
	is_stop_streaming_ = true;
}

bool AsioDevice::IsMuted() const {
	return volume_ == 0;
}

uint32_t AsioDevice::GetBufferSize() const noexcept {
	return static_cast<uint32_t>(buffer_size_ * format_.GetChannels());
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

void AsioDevice::ResetCurrentDriver() {
	if (!the_driver_context.drivers) {
		return;
	}
	if (!the_driver_context.device) {
		return;
	}
	the_driver_context.device->ReOpen();
	the_driver_context.drivers.reset();
	XAMP_LOG_DEBUG("Remove ASIO driver");
}

void AsioDevice::RemoveCurrentDriver() {
	if (!is_removed_driver_ && the_driver_context.drivers != nullptr) {
		the_driver_context.drivers->removeCurrentDriver();
		is_removed_driver_ = true;
	}
}

void AsioDevice::SetVolumeLevelScalar(float level) {
}

void AsioDevice::ReOpen() {
	if (the_driver_context.drivers != nullptr) {
		const auto driver_name = the_driver_context.GetDriverName();
		if (the_driver_context.driver_name == driver_name) {
			is_removed_driver_ = false;
			return;
		}		
	}
	if (!the_driver_context.drivers) {
		the_driver_context.drivers = MakeAlign<AsioDrivers>();
	}
	the_driver_context.drivers->removeCurrentDriver();
	const auto driver_name = the_driver_context.GetDriverName();
	if (!the_driver_context.drivers->loadDriver(const_cast<char*>(device_id_.c_str()))) {		
		the_driver_context.drivers.reset();
		throw DeviceNotFoundException(driver_name);
	}
	the_driver_context.driver_name = driver_name;
	is_removed_driver_ = false;
}

std::tuple<int32_t, int32_t> AsioDevice::GetDeviceBufferSize() const {
	long min_size = 0;
	long max_size = 0;
	long prefer_size = 0;
	long granularity = 0;
	AsioIfFailedThrow(::ASIOGetBufferSize(&min_size, &max_size, &prefer_size, &granularity));

	XAMP_LOG_D(logger_, "min_size:{} max_size:{} prefer_size:{} granularity:{}.",
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
	for (auto& info : the_driver_context.buffer_infos) {
		info.isInput = ASIOFalse;
		info.channelNum = num_channel;
		info.buffers[0] = nullptr;
		info.buffers[1] = nullptr;
		++num_channel;
	}

	the_driver_context.asio_callbacks.bufferSwitch = OnBufferSwitchCallback;
	the_driver_context.asio_callbacks.sampleRateDidChange = OnSampleRateChangedCallback;
	the_driver_context.asio_callbacks.asioMessage = OnAsioMessagesCallback;
	the_driver_context.asio_callbacks.bufferSwitchTimeInfo = OnBufferSwitchTimeInfoCallback;
	the_driver_context.data_context.volume_factor = LinearToLog(volume_);

	const auto result = ::ASIOCreateBuffers(
		the_driver_context.buffer_infos.data(),
	    output_format.GetChannels(),
	    buffer_size,
	    &the_driver_context.asio_callbacks);
	if (result != ASE_OK) {
		AsioIfFailedThrow(::ASIOCreateBuffers(the_driver_context.buffer_infos.data(),
			output_format.GetChannels(),
			prefer_size,
			&the_driver_context.asio_callbacks));
		buffer_size_ = prefer_size;
	}
	else {
		buffer_size_ = buffer_size;
	}

	for (long i = 0; i < the_driver_context.buffer_infos.size(); ++i) {
		the_driver_context.channel_infos[i].channel =
			the_driver_context.buffer_infos[i].channelNum;
		the_driver_context.channel_infos[i].isInput =
			the_driver_context.buffer_infos[i].isInput;
		AsioIfFailedThrow(::ASIOGetChannelInfo(&the_driver_context.channel_infos[i]));
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
	case ASIOSTInt16LSB:
		//format_.SetByteFormat(ByteFormat::SINT16);
		XAMP_LOG_D(logger_, "Driver support format: 16 bit.");
		//break;
		throw AsioException(Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT);
	case ASIOSTInt24LSB:   // used for 20 bits as well
	case ASIOSTInt32LSB24: // 32 bit data with 24 bit alignment
	case ASIOSTInt24MSB:   // used for 20 bits as well
	case ASIOSTInt32MSB24: // 32 bit data with 24 bit alignment
		//format_.SetByteFormat(ByteFormat::SINT24);
		XAMP_LOG_D(logger_, "Driver support format: 20 bit.");
		//break;
		throw AsioException(Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT);
	case ASIOSTFloat32LSB: // IEEE 754 32 bit float, as found on Intel x86 architecture
		//format_.SetByteFormat(ByteFormat::FLOAT32);
		XAMP_LOG_D(logger_, "Driver support format: 32 bit float.");
		//break;
		throw AsioException(Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT);
	case ASIOSTFloat64LSB: // IEEE 754 64 bit double float, as found on Intel x86 architecture	
		//format_.SetByteFormat(ByteFormat::FLOAT64);
		XAMP_LOG_D(logger_, "Driver support format: 64 bit float.");
		//break;
		throw AsioException(Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT);
	case ASIOSTFloat32MSB: // IEEE 754 32 bit float, Big Endian architecture 
		//format_.SetByteFormat(ByteFormat::FLOAT32);
		XAMP_LOG_D(logger_, "Driver support format: 32 bit float.");
		//break;
		throw AsioException(Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT);
	case ASIOSTFloat64MSB: // IEEE 754 64 bit double float, Big Endian architecture		
		//format_.SetByteFormat(ByteFormat::FLOAT64);
		XAMP_LOG_D(logger_, "Driver support format: 64 bit float.");
		//break;
		throw AsioException(Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT);
	case ASIOSTInt32MSB:
		//format_.SetByteFormat(ByteFormat::SINT32);
		XAMP_LOG_D(logger_, "Driver support format: ASIOSTInt32LSB.");
		//break;
		throw AsioException(Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT);
	case ASIOSTInt32LSB: 
		format_.SetByteFormat(ByteFormat::SINT32);
		XAMP_LOG_D(logger_, "Driver support format: ASIOSTInt32LSB.");
		break;
		// DSD 8 bit data, 1 sample per byte. No Endianness required.
	case ASIOSTDSDInt8LSB1:
		format_.SetByteFormat(ByteFormat::SINT8);
		XAMP_LOG_D(logger_, "Driver support format: ASIOSTDSDInt8LSB1.");
		break;
	case ASIOSTDSDInt8MSB1:
		format_.SetByteFormat(ByteFormat::SINT8);
		XAMP_LOG_D(logger_, "Driver support format: ASIOSTDSDInt8MSB1.");
		break;
	case ASIOSTDSDInt8NER8:
		//format_.SetByteFormat(ByteFormat::SINT8);
		XAMP_LOG_D(logger_, "Driver support format: ASIOSTDSDInt8NER8.");
		//break;
		throw AsioException(Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT);
	default:
		throw AsioException(Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT);
	}

	XAMP_LOG_D(logger_, "Native DSD support: {}.", IsSupportDsdFormat());

	size_t allocate_bytes = 0;
	if (io_format_ == DsdIoFormat::IO_FORMAT_PCM) {
		// note: 來源的資料格式均為float(4 bytes), 但實際上要輸出的格式不一定4 bytes, 
		// 但是為了滿足來源的資料格式(4 bytes), 所以都要配置float(4 bytes)大小的緩衝區.
		constexpr auto kFloatByteSize = 4;
		allocate_bytes = buffer_size_ * kFloatByteSize * format_.GetChannels();
		buffer_bytes_ = buffer_size_ * kFloatByteSize;
		the_driver_context.data_context = MakeConvert(in_format, format_, buffer_size_);
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
		allocate_bytes = buffer_size_;		
		the_driver_context.data_context = MakeConvert(in_format, format_, channel_buffer_size);
	}

	if (device_buffer_.GetSize() != allocate_bytes) {
		device_buffer_.reset();
		device_buffer_ = MakeBuffer<int8_t>(allocate_bytes);
	}
	if (buffer_.GetSize() != allocate_bytes) {
		buffer_.reset();
		buffer_ = MakeBuffer<int8_t>(allocate_bytes);
	}

	long input_latency = 0;
	long output_latency = 0;
	AsioIfFailedThrow(::ASIOGetLatencies(&input_latency, &output_latency));
	latency_ = GetLatencyMs(output_latency, output_format.GetSampleRate());
	
	the_driver_context.SetPostOutput();

	// Almost driver not support kAsioCanOutputGain, we always return true for software volume control.
	//return (io_format_ == DsdIoFormat::IO_FORMAT_PCM);
	ASIOIoFormat asio_fomrmat{};
	is_hardware_control_volume_ = !(::ASIOFuture(kAsioCanOutputGain, &asio_fomrmat) == ASE_SUCCESS);

	XAMP_LOG_D(logger_, "IO format :{} ", io_format_);
	XAMP_LOG_D(logger_, "Buffer size :{} ", String::FormatBytes(buffer_.GetByteSize()));
	XAMP_LOG_D(logger_, "Driver support post output: {}", the_driver_context.post_output);

	if (latency_ < 15) {
		XAMP_LOG_W(logger_, "Output latency: {} msec too small.", latency_);
	}
}

uint32_t AsioDevice::GetVolume() const {
	return volume_;
}

void AsioDevice::SetVolume(uint32_t volume) const {
	if (is_hardware_control_volume_) {
		return;
	}
	volume_ = std::clamp(volume, static_cast<uint32_t>(0), static_cast<uint32_t>(99));
	XAMP_LOG_D(logger_, "Current volume: {}({} db)", GetVolume(), VolumeToDb(volume_));
}

void AsioDevice::SetMute(bool mute) const {
	if (mute) {
		volume_ = 0;
	}
}

void AsioDevice::FillSilentData() noexcept {
	for (size_t i = 0, j = 0; i < format_.GetChannels(); ++i) {
		MemorySet(the_driver_context.buffer_infos[i].buffers[0],
			0,
			buffer_bytes_);
		MemorySet(the_driver_context.buffer_infos[i].buffers[1],
			0,
			buffer_bytes_);
	}
}

bool AsioDevice::GetPCMSamples(long index, double sample_time, size_t& num_filled_frame) noexcept {
	const auto vol = volume_.load();
	if (the_driver_context.data_context.cache_volume != vol) {
		the_driver_context.data_context.volume_factor = LinearToLog(vol);
		the_driver_context.data_context.cache_volume = vol;
	}

	// PCM mode input float to output format.
	const auto stream_time = static_cast<double>(output_bytes_) / format_.GetAvgBytesPerSec();
	buffer_.Fill(0);

	XAMP_LIKELY(callback_->OnGetSamples(buffer_.Get(), buffer_size_, num_filled_frame, stream_time, sample_time) == DataCallbackResult::CONTINUE) {
		const auto in = reinterpret_cast<const float*>(buffer_.Get());
		InterleaveToPlanar<float, int32_t>::Convert(
			in,
			static_cast<int32_t*>(the_driver_context.buffer_infos[0].buffers[index]),
			static_cast<int32_t*>(the_driver_context.buffer_infos[1].buffers[index]),
			buffer_size_ * format_.GetChannels(),
			the_driver_context.data_context.volume_factor);
		return true;
	} else {
		return false;
	}
}

bool AsioDevice::GetDSDSamples(long index, double sample_time, size_t& num_filled_frame) noexcept {
	// DSD mode input output same format (int8_t).
	const auto avg_byte_per_sec = format_.GetAvgBytesPerSec() / 8;
	const auto stream_time = static_cast<double>(output_bytes_) / avg_byte_per_sec;

	XAMP_LIKELY(callback_->OnGetSamples(buffer_.Get(), buffer_bytes_, num_filled_frame, stream_time, sample_time) == DataCallbackResult::CONTINUE) {
		DataConverter<
			PackedFormat::PLANAR,
			PackedFormat::INTERLEAVED
		>::Convert(
				device_buffer_.Get(),
				buffer_.Get(),
				the_driver_context.data_context);
		for (size_t i = 0, j = 0; i < format_.GetChannels(); ++i) {
			MemoryCopy(the_driver_context.buffer_infos[i].buffers[index],
				&device_buffer_[j++ * buffer_bytes_],
				buffer_bytes_);
		}
		return true;
	} else {
		return false;
	}
}

void AsioDevice::GetSamples(long index, double sample_time) noexcept {
	if (!is_streaming_) {
		XAMP_LOG_D(logger_, "Stream was stopped!1");
		// 填充靜音的資料並通知結束.
		FillSilentData();
		is_stop_streaming_ = true;
		condition_.notify_all();
		the_driver_context.Post();
		return;
	}

	if (is_stop_streaming_) {
		XAMP_LOG_D(logger_, "Stream was stopped!2");
	}
	
	auto cache_played_bytes = output_bytes_.load();
	if (cache_played_bytes == 0) {
		the_driver_context.mmcss.BoostPriority();
	}
	
	cache_played_bytes += buffer_bytes_ * format_.GetChannels();
	output_bytes_ = cache_played_bytes;	

	size_t num_filled_frame = 0;
	const auto got_samples = std::invoke(get_samples_, index, sample_time, num_filled_frame);
	
	if (got_samples) {
		the_driver_context.Post();
	} else {
		FillSilentData();
	}
}

void AsioDevice::OpenStream(AudioFormat const & output_format) {
	ASIODriverInfo asio_driver_info{};
	asio_driver_info.asioVersion = 2;
	asio_driver_info.sysRef = ::GetDesktopWindow();

	// 如果有播放過的話(callbackInfo.device != nullptr), ASIO每次必須要重新建立!
	ReOpen();

	// Get driver name
	auto name_len = (std::min)(sizeof(asio_driver_info.name) - 1, device_id_.length());
	MemoryCopy(asio_driver_info.name, device_id_.c_str(), name_len);

	// Initialize ASIO driver
	auto result = ::ASIOInit(&asio_driver_info);
	if (result == ASE_NotPresent) {
		const auto driver_name = the_driver_context.GetDriverName();
		the_driver_context.drivers.reset();
		throw DeviceNotFoundException(driver_name);
	}
	else if (result != ASE_OK) {
		throw AsioException(result);
	}	

	ASIOIoFormat asio_io_format{};
	if (io_format_ == DsdIoFormat::IO_FORMAT_DSD) {
		asio_io_format.FormatType = kASIODSDFormat;
	}
	else {
		asio_io_format.FormatType = kASIOPCMFormat;
	}

	try {
		AsioIfFailedThrow2(::ASIOFuture(kAsioSetIoFormat, &asio_io_format), ASE_SUCCESS);
	}
	catch (const Exception & e) {
		XAMP_LOG_D(logger_, "ASIOFuture return failure. {}", e.GetErrorMessage());
		// NOTE: DSD format must be support!
		if (output_format.GetFormat() == DataFormat::FORMAT_DSD) {
			throw;
		}
	}

	SetOutputSampleRate(output_format);
	CreateBuffers(output_format);

	output_bytes_ = 0;
	is_stopped_ = false;
	the_driver_context.data_context.cache_volume = 0;
	the_driver_context.device = this;
}

void AsioDevice::SetOutputSampleRate(AudioFormat const & output_format) {
	// Set device sample rate
	const auto error = ::ASIOSetSampleRate(output_format.GetSampleRate());
	if (error == ASE_NotPresent) {
		throw DeviceUnSupportedFormatException(output_format);
	}
	AsioIfFailedThrow(error);
	XAMP_LOG_D(logger_, "Set device sample rate: {}.", output_format.GetSampleRate());

	clock_source_.resize(kClockSourceSize);

	auto num_clock_source = static_cast<long>(clock_source_.size());

	// Get device clock source
	AsioIfFailedThrow(::ASIOGetClockSources(clock_source_.data(), &num_clock_source));

	auto is_current_source_set = false;
	if (num_clock_source > 0) {
		const auto itr = std::ranges::find_if(clock_source_,
		                                      [](const auto& source) {
			                                      return source.isCurrentSource;
		                                      });
		is_current_source_set = itr != clock_source_.end();
	}

	if (!is_current_source_set && num_clock_source > 1) {
		XAMP_LOG_D(logger_, "Set device clock source: {}.", clock_source_[0].name);
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

	XAMP_LOG_D(logger_, "StopStream");

	is_streaming_ = false;
	std::unique_lock lock{ mutex_ };

	while (wait_for_stop_stream && !is_stop_streaming_) {
		condition_.wait(lock);
	}

	// TODO: Workaround!
	// 等待10ms這段期間OnBufferSwitch會填充靜音的資料.	
	MSleep(std::chrono::milliseconds(10));
	AsioIfFailedThrow(::ASIOStop());
}

void AsioDevice::CloseStream() {
	XAMP_LOG_D(logger_, "CloseStream");

	if (!is_removed_driver_) {
		AsioIfFailedThrow(::ASIOStop());		
		AsioIfFailedThrow(::ASIODisposeBuffers());
		is_removed_driver_ = true;
	}
	callback_ = nullptr;	
}

void AsioDevice::StartStream() {
	if (io_format_ == DsdIoFormat::IO_FORMAT_PCM) {
		get_samples_ = bind_front(&AsioDevice::GetPCMSamples, this);
	}
	else {
		get_samples_ = bind_front(&AsioDevice::GetDSDSamples, this);
	}

	the_driver_context.mmcss.RevertPriority();
	is_streaming_ = true;
	is_stop_streaming_ = false;
	the_driver_context.buffer_switch_stopwatch.Reset();
	AsioIfFailedThrow(::ASIOStart());
}

bool AsioDevice::IsStreamRunning() const noexcept {
	return is_streaming_;
}

void AsioDevice::SetStreamTime(double stream_time) noexcept {
	if (io_format_ == DsdIoFormat::IO_FORMAT_PCM) {
		output_bytes_ = static_cast<int64_t>(stream_time * format_.GetAvgBytesPerSec());
	}
	else {
		const auto avg_byte_per_sec = format_.GetAvgBytesPerSec() / 8;
		output_bytes_ = stream_time * avg_byte_per_sec;
	}
}

double AsioDevice::GetStreamTime() const noexcept {
	if (io_format_ == DsdIoFormat::IO_FORMAT_PCM) {
		return static_cast<double>(output_bytes_) / format_.GetAvgBytesPerSec();
	} else {
		const auto avg_byte_per_sec = format_.GetAvgBytesPerSec() / 8;
		return static_cast<double>(output_bytes_) / avg_byte_per_sec;
	}
}

PackedFormat AsioDevice::GetPackedFormat() const noexcept {
	return PackedFormat::PLANAR;
}

ASIOTime* AsioDevice::OnBufferSwitchTimeInfoCallback(ASIOTime* timeInfo, long index, ASIOBool processNow) noexcept {
	return timeInfo;
}

void AsioDevice::OnBufferSwitchCallback(long index, ASIOBool processNow) {
	if (!the_driver_context.mmcss_registered) {
		the_driver_context.mmcss.BoostPriority(kMmcssProfileProAudio, MmcssThreadPriority::MMCSS_THREAD_PRIORITY_NORMAL);
		the_driver_context.mmcss_registered = true;
	}

	ASIOTime time_info{ 0 };
	if (::ASIOGetSamplePosition(&time_info.timeInfo.samplePosition, &time_info.timeInfo.systemTime) == ASE_OK) {
		time_info.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;
	}
	
	the_driver_context.device->OnBufferSwitchTimeInfoCallback(&time_info, index, processNow);
	double sample_time = 0;
	if (time_info.timeInfo.flags & kSamplePositionValid) {
		sample_time = ASIO64toDouble(time_info.timeInfo.samplePosition)
		/ the_driver_context.device->format_.GetSampleRate();
	}
	constexpr auto kMinimalLatencyMs = 15;
	const auto elapsed = the_driver_context.buffer_switch_stopwatch.Elapsed<std::chrono::milliseconds>().count();
	if (the_driver_context.device->latency_ > kMinimalLatencyMs) {
		if (elapsed > the_driver_context.device->latency_) {
			XAMP_LOG_D(the_driver_context.device->logger_, "Wait event timeout! {}ms", elapsed);
		}
	}

	the_driver_context.device->GetSamples(index, sample_time);
	the_driver_context.buffer_switch_stopwatch.Reset();
}

long AsioDevice::OnAsioMessagesCallback(long selector, long value, void* message, double* opt) {
	long ret = 0;

	if (the_driver_context.device != nullptr) {
		XAMP_LOG_INFO("Select:{} value:{}", selector, value);
	}
	
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
		the_driver_context.device->AbortStream();		
		the_driver_context.device->condition_.notify_one();
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
		the_driver_context.is_xrun = true;
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

	the_driver_context.device->StopStream();
	the_driver_context.device->callback_->OnError(SampleRateChangedException());
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif