#include <output_device/win32/exclusivewasapidevice.h>

#ifdef XAMP_OS_WIN
#include <output_device/iaudiocallback.h>
#include <output_device/win32/mmcss.h>
#include <output_device/win32/comexception.h>
#include <output_device/win32/wasapi.h>

#include <base/volume.h>
#include <base/logger.h>
#include <base/str_utilts.h>
#include <base/stopwatch.h>
#include <base/scopeguard.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(ExclusiveWasapiDevice);

using namespace helper;

namespace {	

	/*
	* Set WAVEFORMATEX from AudioFormat and valid bits samples
	*
	* @param input_format: input format
	* @param audio_format: audio format
	*/
	void SetWaveformatEx(WAVEFORMATEX* input_format, const AudioFormat& audio_format, const int32_t valid_bits_samples) {
		XAMP_EXPECTS(input_format != nullptr);
		XAMP_EXPECTS(audio_format.getChannels() == AudioFormat::kMaxChannel);
		XAMP_EXPECTS(valid_bits_samples > 0);

		// Check if this is correct	
		XAMP_EXPECTS(input_format->cbSize == sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX));
		auto& format = *reinterpret_cast<WAVEFORMATEXTENSIBLE*>(input_format);

		// CopyFrom from AudioFormat
		format.Format.nChannels = audio_format.getChannels();
		format.Format.nSamplesPerSec = audio_format.getSampleRate();
		format.Format.nAvgBytesPerSec = audio_format.getAvgBytesPerSec();
		format.Format.nBlockAlign = audio_format.getBlockAlign();
		format.Samples.wValidBitsPerSample = valid_bits_samples;

		if (audio_format.getChannels() <= 2
			&& ((audio_format.getBitsPerSample() == 16) || (audio_format.getBitsPerSample() == 8))) {
			// If this is a PCM format, we can set the wFormatTag to WAVE_FORMAT_PCM
			// and the SubFormat to KSDATAFORMAT_SUBTYPE_PCM. Otherwise, we need to
			// set the wFormatTag to WAVE_FORMAT_PCM.
			format.Format.cbSize = 0;
			format.Format.wFormatTag = WAVE_FORMAT_PCM;
			format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			format.Format.wBitsPerSample = audio_format.getBitsPerSample();
		}
		else {
			// This is 24/32 bit float format setting.
			// If this is a PCM format, we can set the wFormatTag to WAVE_FORMAT_PCM
			// and the SubFormat to KSDATAFORMAT_SUBTYPE_PCM. Otherwise, we need to
			// set the wFormatTag to WAVE_FORMAT_EXTENSIBLE and the SubFormat to
			// KSDATAFORMAT_SUBTYPE_IEEE_FLOAT.
			format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
			format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
			format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			if (audio_format.getBitsPerSample() == 24) 
				format.Format.wBitsPerSample = 24;
			else
				format.Format.wBitsPerSample = 32;
		}
		format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
	}

	uint32_t BackwardAligned(const uint32_t bytes_frame, const uint32_t align_size) {
		return (bytes_frame - (align_size ? (bytes_frame % align_size) : 0));
	}

	template <typename Predicate>
	uint32_t CalcAlignedFramePerBuffer(const uint32_t frames, const uint32_t block_align, Predicate f) {
		constexpr UINT32 kHdAudioPacketSize = 128;

		const auto bytes_frame = frames * block_align;
		auto new_bytes_frame = f(bytes_frame, kHdAudioPacketSize);
		if (new_bytes_frame < kHdAudioPacketSize) {
			new_bytes_frame = kHdAudioPacketSize;
		}

		auto new_frames = new_bytes_frame / block_align;
		const UINT32 packets = new_bytes_frame / kHdAudioPacketSize;
		new_bytes_frame = packets * kHdAudioPacketSize;
		new_frames = new_bytes_frame / block_align;
		return new_frames;
	}

	template <typename Predicate>
	int32_t MakeAlignedPeriod(const AudioFormat& format, uint32_t frames_per_latency, Predicate f) {
		return CalcAlignedFramePerBuffer(frames_per_latency, format.getBlockAlign(), f);
	}

	constexpr auto kAudioRenderClientID = __uuidof(IAudioRenderClient);
	constexpr auto kAudioEndpointVolumeID = __uuidof(IAudioEndpointVolume);
	constexpr auto kAudioClient3ID = __uuidof(IAudioClient3);
	constexpr auto kAudioClockID = __uuidof(IAudioClock);

	// A total typical delay of 35 ms contains three parts:
	// 1. Audio endpoint device period (~10 ms).
	// 2. Stream latency between the buffer and endpoint device (~5 ms).
	// 3. Endpoint buffer (~20 ms to ensure glitch-free rendering).
	constexpr REFERENCE_TIME kGlitchFreePeriod = 350000;
	constexpr std::chrono::milliseconds kGlitchFreeDuration{35};
}

ExclusiveWasapiDevice::ExclusiveWasapiDevice(const CComPtr<IMMDevice>& device)
	: ignore_wait_slow_(false)
	, is_2432_format_(true)
	, is_running_(false)
	, thread_priority_(MmcssThreadPriority::MMCSS_THREAD_PRIORITY_NORMAL)
	, buffer_frames_(0)
	, device_frequency_(0)
	, buffer_period_(0)
	, volume_support_mask_(0)
	, stream_time_(0)
	, sample_ready_(nullptr)
	, mmcss_name_(kMmcssProfileProAudio)
	, aligned_period_(0)
	, device_(device)
	, callback_(nullptr)
	, logger_(XampLoggerFactory.getLogger(XAMP_LOG_NAME(ExclusiveWasapiDevice))) {
}

ExclusiveWasapiDevice::~ExclusiveWasapiDevice() {
    try {
        closeStream();
        sample_ready_.reset();
    } catch (...) {
    }
}
	
void ExclusiveWasapiDevice::setAlignedPeriod(REFERENCE_TIME device_period, const AudioFormat &output_format) {
	// From device period to buffer size.
	buffer_frames_ = ReferenceTimeToFrames(device_period, output_format.getSampleRate());
	// Make sure the buffer size is a multiple of the HD audio packet size.
	buffer_frames_ = MakeAlignedPeriod(output_format, buffer_frames_, BackwardAligned);
	// Get aligned period from buffer size.
	aligned_period_ = MakeHnsPeriod(buffer_frames_, output_format.getSampleRate());
}

void ExclusiveWasapiDevice::initialDeviceFormat(const AudioFormat & output_format, const uint32_t valid_bits_samples) {
	// Set the mix format.
	SetWaveformatEx(mix_format_, output_format, valid_bits_samples);

	REFERENCE_TIME default_device_period = 0;
	REFERENCE_TIME minimum_device_period = 0;

	if (buffer_period_ == 0) {
		// If buffer_period_ is not set, use default device period.
		hrIfFailThrow(client_->GetDevicePeriod(&default_device_period, &minimum_device_period));
		default_device_period = kGlitchFreePeriod;
	} else {
		default_device_period = buffer_period_;
	}

	// Exclusive WASAPI must be set	aligned period.
	// setAlignedPeriod will set buffer_frames_ and aligned_period_.
	setAlignedPeriod(default_device_period, output_format);

	XAMP_LOG_D(logger_, "Device period: default: {:.2f} msec, min: {:.2f} msec.",
		Nano100ToMillis(default_device_period),
		Nano100ToMillis(minimum_device_period));
	XAMP_LOG_D(logger_, "initial aligned period: {:.2f} msec, buffer frames: {}.",
		Nano100ToMillis(aligned_period_), 
		getBufferSize());

	// initial device format and aligned period.
	const auto hr = client_->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,
	                                    AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
	                                    aligned_period_,
	                                    aligned_period_,
	                                    mix_format_,
	                                    nullptr);

	if (FAILED(hr)) {
		if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
			throw DeviceUnSupportedFormatException(output_format);
		}
		if (hr == AUDCLNT_E_DEVICE_IN_USE) {
			throw DeviceInUseException();
		}
		hrIfFailThrow(hr);
	}
	
	CComPtr<IAudioEndpointVolume> endpoint_volume;

	hrIfFailThrow(device_->Activate(kAudioEndpointVolumeID,
		CLSCTX_INPROC_SERVER,
		nullptr,
		reinterpret_cast<void**>(&endpoint_volume)
	));

	// Check hardware support volume control.
	hrIfFailThrow(endpoint_volume->QueryHardwareSupport(&volume_support_mask_));
	
	if (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_VOLUME) {
		XAMP_LOG_D(logger_, "Hardware support volume control.");
	}
	else {
		XAMP_LOG_D(logger_, "Hardware not support volume control.");
	}
	if (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_MUTE) {
		XAMP_LOG_D(logger_, "Hardware support volume mute.");
	}
	else {
		XAMP_LOG_D(logger_, "Hardware not support volume mute.");
	}
	if (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_METER) {
		XAMP_LOG_D(logger_, "Hardware support volume meter.");
	}
	else {
		XAMP_LOG_D(logger_, "Hardware not support volume meter.");
	}
}

void ExclusiveWasapiDevice::openStream(const AudioFormat& output_format) {    
	stream_time_ = 0;

	if (!client_) {
		XAMP_LOG_D(logger_, "Active device format: {}.", output_format);

        hrIfFailThrow(device_->Activate(kAudioClient3ID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&client_)));

		hrIfFailThrow(device_->Activate(kAudioEndpointVolumeID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&endpoint_volume_)));
		if (isBitstreamVolumeLocked()) {
			forceBitstreamEndpointVolume();
		}

		hrIfFailThrow(client_->GetMixFormat(&mix_format_));

		HRESULT hr = S_OK;
		if (output_format.getByteFormat() == ByteFormat::SINT32) {
			hr = client_->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, mix_format_, nullptr);

			auto is_32bit_format = false;

			// Device report 32 bit format but valid bits per sample is 24 bits.
			if (mix_format_->wFormatTag == WAVE_FORMAT_EXTENSIBLE
				&& mix_format_->cbSize == sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
				const auto& driver_format = *reinterpret_cast<WAVEFORMATEXTENSIBLE*>(mix_format_.m_pData);
				XAMP_LOG_DEBUG("Driver report wBitsPerSample:{} wValidBitsPerSample:{}", 
					driver_format.Format.wBitsPerSample, driver_format.Samples.wValidBitsPerSample);
				if (driver_format.Format.wBitsPerSample == 32) {
					is_32bit_format = driver_format.Format.wBitsPerSample == driver_format.Samples.wValidBitsPerSample;
				}
			}

			// If the format is not supported, try to fallback to valid format.
			if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
				if (!is_32bit_format) {
					constexpr uint32_t kValidBitPerSamples = 24;
					initialDeviceFormat(output_format, kValidBitPerSamples);
					XAMP_LOG_D(logger_, "Use valid output format: {}.", kValidBitPerSamples);
					is_2432_format_ = true;
				}
				else {
					initialDeviceFormat(output_format, 32);
					XAMP_LOG_D(logger_, "Fallback use valid output format: 32.");
					is_2432_format_ = false;
				}
			}
			else if (SUCCEEDED(hr)) {
				// The format is supported.
				if (is_32bit_format) {
					initialDeviceFormat(output_format, 32);
					is_2432_format_ = false;
				}
				else {
					constexpr uint32_t kValidBitPerSamples = 24;
					initialDeviceFormat(output_format, kValidBitPerSamples);
					is_2432_format_ = true;
				}
			}
			else {
				// Some other error occurred.
				hrIfFailThrow(hr);
			}

			if (getIoFormat() == DsdIoFormat::IO_FORMAT_DSD
				|| getIoFormat() == DsdIoFormat::IO_FORMAT_DOP) {
				is_2432_format_ = true;
			}
		} else {
			is_2432_format_ = false;
			switch (output_format.getBitsPerSample()) {
			case 16:
				initialDeviceFormat(output_format, 16);
				break;
			case 24:
				initialDeviceFormat(output_format, 24);
				break;
			}		
		}
    }
	// reset device state.
    hrIfFailThrow(client_->Reset());

	// Get device render client. 
    hrIfFailThrow(client_->GetService(kAudioRenderClientID,
		reinterpret_cast<void**>(&render_client_)));

	// Get device clock.
	hrIfFailThrow(client_->GetService(kAudioClockID,
		reinterpret_cast<void**>(&clock_)));

	if (clock_) {
		hrIfFailThrow(clock_->GetFrequency(&device_frequency_));
		using namespace std::chrono;
		auto buffer_duration_ms =
			duration_cast<milliseconds>(nanoseconds(aligned_period_ * 100));
		glitch_detector_.reset(device_frequency_, buffer_duration_ms);
	}

	// create sample ready event handle.
	if (!sample_ready_) {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		hrIfFailThrow(client_->SetEventHandle(sample_ready_.get()));
	}

	rt_work_queue_ = MakeWasapiWorkQueue(mmcss_name_, this, &ExclusiveWasapiDevice::onInvoke);

	// Calculate buffer size.
    const size_t buffer_size = buffer_frames_ * output_format.getChannels();
	if (buffer_.size() != buffer_size) {
		buffer_ = makeBuffer<float>(buffer_size);		
	}

	// create convert function.
    data_convert_ = makeConvert(buffer_frames_);
	XAMP_LOG_D(logger_, "WASAPI internal buffer: {}.", String::formatBytes(buffer_.getByteSize()));
}

void ExclusiveWasapiDevice::setSchedulerService(std::wstring const &mmcss_name, MmcssThreadPriority thread_priority) {
	XAMP_EXPECTS(!mmcss_name.empty());
	thread_priority_ = thread_priority;
	mmcss_name_ = mmcss_name;
}

void ExclusiveWasapiDevice::reportError(HRESULT hr) {
	if (FAILED(hr)) {
		callback_->onError(com_to_system_error(hr));
		is_running_ = false;
	}	
}

bool ExclusiveWasapiDevice::getSample(bool is_silence) {
	XAMP_ENSURES(render_client_ != nullptr);
	XAMP_ENSURES(callback_ != nullptr);

	Accumulator glitch_accumulator;
	BYTE* data = nullptr;

	if (!is_silence && clock_ && device_frequency_ != 0) {
		UINT64 position = 0;
		UINT64 qpc_position = 0;
		if (SUCCEEDED(clock_->GetPosition(&position, &qpc_position))) {
			if (auto g = glitch_detector_.update(position, qpc_position)) {
				glitch_accumulator.Add(g.value());				
			}
		}
	}

	AudioGlitchInfo glitch_info = glitch_accumulator.GetAndReset();
	callback_->onGlitch(glitch_info.duration, glitch_info.count);

	// Get buffer from device.
	auto hr = render_client_->GetBuffer(buffer_frames_, &data);
	if (FAILED(hr)) {
		return false;
	}

	// Calculate stream time.
	const auto stream_time = stream_time_ + buffer_frames_;
	stream_time_ = stream_time;
	const auto stream_time_float = static_cast<double>(stream_time)
	/ static_cast<double>(mix_format_->nSamplesPerSec);

	// Calculate sample time.
	const auto sample_time = GetStreamPosInMilliseconds(clock_) / 1000.0;

	DWORD flags = is_silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0;

	// Get sample from callback.
	size_t num_filled_frames = 0;
	if (callback_->onGetSamples(buffer_.get(),
		buffer_frames_,
		num_filled_frames,
		stream_time_float,
		sample_time) == DataCallbackResult::CONTINUE) {
		bool result = true;
		if (num_filled_frames != buffer_frames_) {
			flags = AUDCLNT_BUFFERFLAGS_SILENT;
			result = false;
		}
		convert_.convert(data, buffer_.get(), data_convert_);
		hr = render_client_->ReleaseBuffer(buffer_frames_, flags);
		return result;
	}
	// EOF data
	hr = render_client_->ReleaseBuffer(buffer_frames_, AUDCLNT_BUFFERFLAGS_SILENT);
	return false;
}

void ExclusiveWasapiDevice::setAudioCallback(IAudioCallback* callback) {
	XAMP_EXPECTS(callback != nullptr);
	callback_ = callback;
}

bool ExclusiveWasapiDevice::isStreamOpen() const {
    return client_ != nullptr;
}

bool ExclusiveWasapiDevice::isStreamRunning() const {
    return is_running_;
}

void ExclusiveWasapiDevice::closeStream() {
	XAMP_LOG_D(logger_, "closeStream is_running_: {}", is_running_);

	if (rt_work_queue_) {
		rt_work_queue_->destroy();
		rt_work_queue_.Release();
	}
	sample_ready_.close();
	render_client_.Release();
	clock_.Release();
	endpoint_volume_.Release();
	client_.Release();
	mix_format_.Free();
}

void ExclusiveWasapiDevice::abortStream() {
}

void ExclusiveWasapiDevice::setIoFormat(DsdIoFormat format) {
	io_format_ = format;
}

DsdIoFormat ExclusiveWasapiDevice::getIoFormat() const {
	return io_format_;
}

void ExclusiveWasapiDevice::stopStream(bool wait_for_stop_stream) {
	std::unique_lock lock{ mutex_ };

	XAMP_LOG_D(logger_, "stopStream is_running_: {}", is_running_);
	ignore_wait_slow_ = true;
	is_running_ = false;
	if (rt_work_queue_) {
		rt_work_queue_->destroy();
	}
	if (client_) {
		hrIfFailThrow(client_->Stop());
	}
}

void ExclusiveWasapiDevice::startStream() {
	std::unique_lock lock{ mutex_ };

	XAMP_LOG_D(logger_, "startStream!");

	if (!client_ || !render_client_) {
		throw_translated_com_error(AUDCLNT_E_NOT_INITIALIZED);
	}

	XAMP_ENSURES(sample_ready_);


	// TODO: Add check 24/32 bit format.
	convert_.setFormat(mix_format_->wBitsPerSample, is_2432_format_);

	// Must be active device and prefill buffer.
	getSample(true);

	try {
		rt_work_queue_->LoadStream();
		rt_work_queue_->WaitAsync(sample_ready_.get());
		is_running_ = true;
		hrIfFailThrow(client_->Start());
	}
	catch (...) {
		is_running_ = false;
		if (rt_work_queue_) {
			rt_work_queue_->destroy();
		}
		throw;
	}
}

HRESULT ExclusiveWasapiDevice::onInvoke(IMFAsyncResult*) {
	if (!is_running_ || rt_work_queue_ == nullptr) {
		return S_OK;
	}

	try {
		if (!getSample(false)) {
			is_running_ = false;
			if (client_) {
				client_->Stop();
			}
			return S_OK;
		}
		rt_work_queue_->WaitAsync(sample_ready_.get());
	}
	catch (const std::exception& e) {
		XAMP_LOG_D(logger_, e.what());
		if (callback_ != nullptr) {
			callback_->onError(e);
		}
		is_running_ = false;
		if (client_) {
			client_->Stop();
		}
	}
	return S_OK;
}
void ExclusiveWasapiDevice::setStreamTime(const double stream_time) {
	stream_time_ = static_cast<int64_t>(stream_time * static_cast<double>(mix_format_->nSamplesPerSec));
}

double ExclusiveWasapiDevice::getStreamTime() const {
    return stream_time_ / static_cast<double>(mix_format_->nSamplesPerSec);
}

uint32_t ExclusiveWasapiDevice::getVolume() const {
	if (isBitstreamVolumeLocked()) {
		return 100;
	}
	if (!isHardwareControlVolume()) {
		return gainToVolumeLevel(data_convert_.volume_factor);
	}
	auto volume_scalar = 0.0F;
	hrIfFailThrow(endpoint_volume_->GetMasterVolumeLevelScalar(&volume_scalar));
	return static_cast<uint32_t>(volume_scalar * 100.0F);
}

void ExclusiveWasapiDevice::setVolume(uint32_t volume) const {
	if (isBitstreamVolumeLocked()) {
		if (volume != 100) {
			XAMP_LOG_D(logger_, "Ignore exclusive WASAPI volume {} in DSD bitstream mode to keep data intact.", volume);
			if (callback_ != nullptr) {
				callback_->onVolumeChange(100);
			}
		}
		forceBitstreamEndpointVolume();
		return;
	}

	if (!isHardwareControlVolume()) {
		data_convert_.volume_factor = volumeLevelToGain(volume);
		return;
	}

	// 將音量限制在0~100%之間
	volume = std::clamp(volume, static_cast<uint32_t>(0), static_cast<uint32_t>(100));

	// 如果目前為靜音狀態，先解靜音
	auto is_mute = FALSE;
	hrIfFailThrow(endpoint_volume_->GetMute(&is_mute));
	if (is_mute) {
		hrIfFailThrow(endpoint_volume_->SetMute(FALSE, nullptr));
	}

	// 將百分比轉換為線性比例(0.0f ~ 1.0f)
	float target_volume_scale = static_cast<float>(volume) / 100.0f;

	// 直接以線性比例設定音量
	hrIfFailThrow(endpoint_volume_->SetMasterVolumeLevelScalar(target_volume_scale, nullptr));

	// 若需檢查當前dB值，可呼叫GetMasterVolumeLevel()取得
	float db_volume = 0.0f;
	hrIfFailThrow(endpoint_volume_->GetMasterVolumeLevel(&db_volume));

	XAMP_LOG_D(logger_,
		"Set volume to {}%, linear scale: {:.2f}, current: {:.2f} dB.",
		volume, target_volume_scale, db_volume);
}

void ExclusiveWasapiDevice::setVolumeLevelScalar(float level) {
	hrIfFailThrow(endpoint_volume_->SetMasterVolumeLevelScalar(level, nullptr));
}

bool ExclusiveWasapiDevice::isMuted() const {
	if (isBitstreamVolumeLocked()) {
		return false;
	}
	auto is_mute = FALSE;
	hrIfFailThrow(endpoint_volume_->GetMute(&is_mute));
	return is_mute;
}

void ExclusiveWasapiDevice::setMute(const bool mute) const {
	if (isBitstreamVolumeLocked()) {
		if (mute) {
			XAMP_LOG_D(logger_, "Ignore mute in exclusive WASAPI DSD bitstream mode to keep data intact.");
			if (callback_ != nullptr) {
				callback_->onVolumeChange(100);
			}
			return;
		}
		forceBitstreamEndpointVolume();
		return;
	}
	hrIfFailThrow(endpoint_volume_->SetMute(mute, nullptr));
}

PackedFormat ExclusiveWasapiDevice::getPackedFormat() const {
    return PackedFormat::INTERLEAVED;
}

uint32_t ExclusiveWasapiDevice::getBufferSize() const {
	return buffer_frames_ * mix_format_->nChannels;
}

bool ExclusiveWasapiDevice::isHardwareControlVolume() const {
	const auto hw_support = (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_VOLUME)
		&& (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_MUTE);
	return hw_support;
}

bool ExclusiveWasapiDevice::isBitstreamVolumeLocked() const {
	return io_format_ == DsdIoFormat::IO_FORMAT_DSD
		|| io_format_ == DsdIoFormat::IO_FORMAT_DOP;
}

void ExclusiveWasapiDevice::forceBitstreamEndpointVolume() const {
	if (endpoint_volume_ == nullptr) {
		return;
	}
	hrIfFailThrow(endpoint_volume_->SetMute(FALSE, nullptr));
	hrIfFailThrow(endpoint_volume_->SetMasterVolumeLevelScalar(1.0f, nullptr));
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
