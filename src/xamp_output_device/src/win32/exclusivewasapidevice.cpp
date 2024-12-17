#include <output_device/win32/exclusivewasapidevice.h>

#ifdef XAMP_OS_WIN
#include <output_device/iaudiocallback.h>
#include <output_device/win32/mmcss.h>
#include <output_device/win32/comexception.h>
#include <output_device/win32/wasapi.h>

#include <base/math.h>
#include <base/executor.h>
#include <base/ithreadpoolexecutor.h>
#include <base/logger.h>
#include <base/str_utilts.h>
#include <base/stopwatch.h>
#include <base/logger_impl.h>
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
	void SetWaveformatEx(WAVEFORMATEX* input_format, const AudioFormat& audio_format, const int32_t valid_bits_samples) noexcept {
		XAMP_EXPECTS(input_format != nullptr);
		XAMP_EXPECTS(audio_format.GetChannels() == AudioFormat::kMaxChannel);
		XAMP_EXPECTS(valid_bits_samples > 0);

		// Check if this is correct	
		XAMP_EXPECTS(input_format->cbSize == sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX));
		auto& format = *reinterpret_cast<WAVEFORMATEXTENSIBLE*>(input_format);

		// CopyFrom from AudioFormat
		format.Format.nChannels = audio_format.GetChannels();
		format.Format.nSamplesPerSec = audio_format.GetSampleRate();
		format.Format.nAvgBytesPerSec = audio_format.GetAvgBytesPerSec();
		format.Format.nBlockAlign = audio_format.GetBlockAlign();
		format.Samples.wValidBitsPerSample = valid_bits_samples;

		if (audio_format.GetChannels() <= 2
			&& ((audio_format.GetBitsPerSample() == 16) || (audio_format.GetBitsPerSample() == 8))) {
			// If this is a PCM format, we can set the wFormatTag to WAVE_FORMAT_PCM
			// and the SubFormat to KSDATAFORMAT_SUBTYPE_PCM. Otherwise, we need to
			// set the wFormatTag to WAVE_FORMAT_PCM.
			format.Format.cbSize = 0;
			format.Format.wFormatTag = WAVE_FORMAT_PCM;
			format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			format.Format.wBitsPerSample = audio_format.GetBitsPerSample();
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
			format.Format.wBitsPerSample = 32;
		}
		format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
	}

	uint32_t BackwardAligned(const uint32_t bytes_frame, const uint32_t align_size) noexcept {
		return (bytes_frame - (align_size ? (bytes_frame % align_size) : 0));
	}

	template <typename Predicate>
	uint32_t CalcAlignedFramePerBuffer(const uint32_t frames, const uint32_t block_align, Predicate f) noexcept {
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
	int32_t MakeAlignedPeriod(const AudioFormat& format, uint32_t frames_per_latency, Predicate f) noexcept {
		return CalcAlignedFramePerBuffer(frames_per_latency, format.GetBlockAlign(), f);
	}

	bool Is16BitSamples(const CComHeapPtr<WAVEFORMATEX> &format) {
		return format->wBitsPerSample == 16;
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

ExclusiveWasapiDevice::ExclusiveWasapiDevice(const std::shared_ptr<IThreadPoolExecutor>& thread_pool, const CComPtr<IMMDevice> & device)
	: raw_mode_(false)
	, ignore_wait_slow_(false)
	, is_2432_format_(true)
	, is_running_(false)
	, thread_priority_(MmcssThreadPriority::MMCSS_THREAD_PRIORITY_NORMAL)
	, buffer_frames_(0)
	, buffer_duration_ms_(0)
	, buffer_period_(0)
	, volume_support_mask_(0)
	, stream_time_(0)
	, sample_ready_(nullptr)
	, mmcss_name_(kMmcssProfileProAudio)
	, aligned_period_(0)
	, device_(device)
	, callback_(nullptr)
	, logger_(XampLoggerFactory.GetLogger(kExclusiveWasapiDeviceLoggerName))
	, thread_pool_(thread_pool) {
}

ExclusiveWasapiDevice::~ExclusiveWasapiDevice() {
    try {
        CloseStream();
        sample_ready_.reset();
    } catch (...) {
    }
}
	
void ExclusiveWasapiDevice::SetAlignedPeriod(REFERENCE_TIME device_period, const AudioFormat &output_format) {
	// From device period to buffer size.
	buffer_frames_ = ReferenceTimeToFrames(device_period, output_format.GetSampleRate());
	// Make sure the buffer size is a multiple of the HD audio packet size.
	buffer_frames_ = MakeAlignedPeriod(output_format, buffer_frames_, BackwardAligned);
	// Get aligned period from buffer size.
	aligned_period_ = MakeHnsPeriod(buffer_frames_, output_format.GetSampleRate());
}

void ExclusiveWasapiDevice::InitialDeviceFormat(const AudioFormat & output_format, const uint32_t valid_bits_samples) {
	// Set the mix format.
	SetWaveformatEx(mix_format_, output_format, valid_bits_samples);

	REFERENCE_TIME default_device_period = 0;
	REFERENCE_TIME minimum_device_period = 0;

	if (buffer_period_ == 0) {
		// If buffer_period_ is not set, use default device period.
		HrIfFailThrow(client_->GetDevicePeriod(&default_device_period, &minimum_device_period));
		default_device_period = kGlitchFreePeriod;
	} else {
		default_device_period = buffer_period_;
	}

	// Exclusive WASAPI must be set	aligned period.
	// SetAlignedPeriod will set buffer_frames_ and aligned_period_.
	SetAlignedPeriod(default_device_period, output_format);

	XAMP_LOG_D(logger_, "Device period: default: {:.2f} msec, min: {:.2f} msec.",
		Nano100ToMillis(default_device_period),
		Nano100ToMillis(minimum_device_period));
	XAMP_LOG_D(logger_, "Initial aligned period: {:.2f} msec, buffer frames: {}.",
		Nano100ToMillis(aligned_period_), 
		GetBufferSize());

	// Initial device format and aligned period.
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
		HrIfFailThrow(hr);
	}
	
	CComPtr<IAudioEndpointVolume> endpoint_volume;

	HrIfFailThrow(device_->Activate(kAudioEndpointVolumeID,
		CLSCTX_INPROC_SERVER,
		nullptr,
		reinterpret_cast<void**>(&endpoint_volume)
	));

	// Check hardware support volume control.
	HrIfFailThrow(endpoint_volume->QueryHardwareSupport(&volume_support_mask_));
	
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

void ExclusiveWasapiDevice::OpenStream(const AudioFormat& output_format) {    
	stream_time_ = 0;

	if (!client_) {
		XAMP_LOG_D(logger_, "Active device format: {}.", output_format);

        HrIfFailThrow(device_->Activate(kAudioClient3ID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&client_)));

        HrIfFailThrow(device_->Activate(kAudioEndpointVolumeID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&endpoint_volume_)));

		HrIfFailThrow(client_->GetMixFormat(&mix_format_));		

		HRESULT hr = S_OK;
		if (output_format.GetByteFormat() == ByteFormat::SINT32) {
			hr = client_->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, mix_format_, nullptr);

			auto is_32bit_format = false;

			if (mix_format_->wFormatTag == WAVE_FORMAT_EXTENSIBLE
				&& mix_format_->cbSize == sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
				const auto& driver_format = *reinterpret_cast<WAVEFORMATEXTENSIBLE*>(mix_format_.m_pData);
				XAMP_LOG_DEBUG("Driver report wBitsPerSample:{} wValidBitsPerSample:{}", 
					driver_format.Format.wBitsPerSample, driver_format.Samples.wValidBitsPerSample);
				if (driver_format.Format.wBitsPerSample == 32) {
					is_32bit_format = driver_format.Format.wBitsPerSample == driver_format.Samples.wValidBitsPerSample;
				}
			}

			if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
				if (!is_32bit_format) {
					constexpr uint32_t kValidBitPerSamples = 24;
					InitialDeviceFormat(output_format, kValidBitPerSamples);
					XAMP_LOG_D(logger_, "Use valid output format: {}.", kValidBitPerSamples);
					is_2432_format_ = true;
				}
				else {
					InitialDeviceFormat(output_format, 32);
					XAMP_LOG_D(logger_, "Fallback use valid output format: 32.");
					is_2432_format_ = false;
				}
			}
			else {
				HrIfFailThrow(hr);
			}

			if (GetIoFormat() == DsdIoFormat::IO_FORMAT_DSD
				|| GetIoFormat() == DsdIoFormat::IO_FORMAT_DOP) {
				is_2432_format_ = true;
			}
		} else {
			InitialDeviceFormat(output_format, 16);
		}
    }

	// Reset device state.
    HrIfFailThrow(client_->Reset());

	// Get device render client.
    HrIfFailThrow(client_->GetService(kAudioRenderClientID,
		reinterpret_cast<void**>(&render_client_)));

	// Get device clock.
	HrIfFailThrow(client_->GetService(kAudioClockID,
		reinterpret_cast<void**>(&clock_)));

	// Create sample ready event handle.
	if (!sample_ready_) {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		HrIfFailThrow(client_->SetEventHandle(sample_ready_.get()));
	}

	// Create thread start event handle.
	if (!thread_start_) {
		thread_start_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
	}

	// Create thread exit event handle.
	if (!thread_exit_) {
		thread_exit_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
	}

	// Create close request event handle.
	if (!close_request_) {
		close_request_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
	}

	// Calculate buffer size.
    const size_t buffer_size = buffer_frames_ * output_format.GetChannels();
	if (buffer_.size() != buffer_size) {
		buffer_ = MakeBuffer<float>(buffer_size);		
	}

	// Create convert function.
    data_convert_ = MakeConvert(output_format, output_format, buffer_frames_);
	XAMP_LOG_D(logger_, "WASAPI internal buffer: {}.", String::FormatBytes(buffer_.GetByteSize()));
}

void ExclusiveWasapiDevice::SetSchedulerService(std::wstring const &mmcss_name, MmcssThreadPriority thread_priority) {
	XAMP_EXPECTS(!mmcss_name.empty());
	thread_priority_ = thread_priority;
	mmcss_name_ = mmcss_name;
}

void ExclusiveWasapiDevice::ReportError(HRESULT hr) noexcept {
	if (FAILED(hr)) {
		callback_->OnError(com_to_system_error(hr));
		is_running_ = false;
	}	
}

bool ExclusiveWasapiDevice::GetSample(bool is_silence) noexcept {
	XAMP_ENSURES(render_client_ != nullptr);
	XAMP_ENSURES(callback_ != nullptr);

	BYTE* data = nullptr;

	// Get buffer from device.
	auto hr = render_client_->GetBuffer(buffer_frames_, &data);
	if (FAILED(hr)) {
		return false;
	}

	// Calculate stream time.
	const auto stream_time = stream_time_ + buffer_frames_;
	stream_time_ = stream_time;
	const auto stream_time_float = static_cast<double>(stream_time) / static_cast<double>(mix_format_->nSamplesPerSec);

	// Calculate sample time.
	const auto sample_time = GetStreamPosInMilliseconds(clock_) / 1000.0;

	DWORD flags = is_silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0;

	// Get sample from callback.
	size_t num_filled_frames = 0;
	if (callback_->OnGetSamples(buffer_.Get(), buffer_frames_, num_filled_frames, stream_time_float, sample_time) == DataCallbackResult::CONTINUE) {
		bool result = true;
		if (num_filled_frames != buffer_frames_) {
			flags = AUDCLNT_BUFFERFLAGS_SILENT;
			result = false;
		}
		std::invoke(convert_, data, buffer_.Get(), data_convert_);		
		hr = render_client_->ReleaseBuffer(buffer_frames_, flags);
		return result;
	}
	// EOF data
	hr = render_client_->ReleaseBuffer(buffer_frames_, AUDCLNT_BUFFERFLAGS_SILENT);
	return false;
}

void ExclusiveWasapiDevice::SetAudioCallback(IAudioCallback* callback) noexcept {
	XAMP_EXPECTS(callback != nullptr);
	callback_ = callback;
}

bool ExclusiveWasapiDevice::IsStreamOpen() const noexcept {
    return client_ != nullptr;
}

bool ExclusiveWasapiDevice::IsStreamRunning() const noexcept {
    return is_running_;
}

void ExclusiveWasapiDevice::CloseStream() {
	XAMP_LOG_D(logger_, "CloseStream is_running_: {}", is_running_);

	sample_ready_.close();
	thread_start_.close();
	thread_exit_.close();
	close_request_.close();
	render_task_ = Task<void>();
	render_client_.Release();
	clock_.Release();
}

void ExclusiveWasapiDevice::AbortStream() noexcept {
}

void ExclusiveWasapiDevice::SetIoFormat(DsdIoFormat format) {
	if (format == DsdIoFormat::IO_FORMAT_DSD || format == DsdIoFormat::IO_FORMAT_DOP) {
		raw_mode_ = true;
	} else {
		raw_mode_ = false;
	}
}

DsdIoFormat ExclusiveWasapiDevice::GetIoFormat() const {
	return raw_mode_ ? DsdIoFormat::IO_FORMAT_DSD : DsdIoFormat::IO_FORMAT_PCM;
}

void ExclusiveWasapiDevice::StopStream(bool wait_for_stop_stream) {
	XAMP_LOG_D(logger_, "StopStream is_running_: {}", is_running_);
	if (!is_running_) {
		return;
	}

	ignore_wait_slow_ = true;

	// Signal thread to exit.
	::SignalObjectAndWait(close_request_.get(), 
		thread_exit_.get(), 
		INFINITE,
		FALSE);

	if (render_task_.valid()) {
		render_task_.get();
	}
	is_running_ = false;
}

void ExclusiveWasapiDevice::StartStream() {
	XAMP_LOG_D(logger_, "StartStream!");

	if (!client_ || !render_client_) {
		throw_translated_com_error(AUDCLNT_E_NOT_INITIALIZED);
	}

	XAMP_ENSURES(sample_ready_);
	XAMP_ENSURES(thread_start_);
	XAMP_ENSURES(thread_exit_);
	XAMP_ENSURES(close_request_);

	// Calculate buffer duration.	
	const std::chrono::milliseconds buffer_duration((buffer_frames_ *
		kMicrosecondsPerSecond /
		mix_format_->nSamplesPerSec) / 1000);
	const std::chrono::milliseconds glitch_time = kGlitchFreeDuration + buffer_duration;

	XAMP_LOG_D(logger_, "WASAPI wait timeout {}msec.", glitch_time.count());
	// Reset event.
	::ResetEvent(close_request_.get());

	if (!Is16BitSamples(mix_format_)) {
		if (!is_2432_format_) {
			convert_ = [](void* data, const float* buffer, const AudioConvertContext& context) {
				DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::Convert(
					static_cast<int32_t*>(data),
					buffer,
					context);
				};
		}
		else {
			convert_ = [](void* data, const float* buffer, const AudioConvertContext& context) {
				DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::ConvertToInt2432(
					static_cast<int32_t*>(data),
					buffer,
					context);
				};
		}
	}
	else {
		convert_ = [](void* data, const float* buffer, const AudioConvertContext& context) {
			DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::Convert(
				static_cast<int16_t*>(data),
				buffer,
				context);
			};
	}

	// Must be active device and prefill buffer.
	GetSample(true);

	render_task_ = Executor::Spawn(thread_pool_.get(), [this, glitch_time](const auto& stop_token) {
		XAMP_LOG_D(logger_, "Start exclusive mode stream task! thread: {}", GetCurrentThreadId());
		DWORD current_timeout = INFINITE;
		Stopwatch watch;
		Mmcss mmcss;

		ignore_wait_slow_ = true;
		is_running_ = true;

		// Boost thread priority.
		mmcss.BoostPriority(mmcss_name_, thread_priority_);

		const std::array<HANDLE, 2> objects{
			sample_ready_.get(),
			close_request_.get()
		};		

		auto thread_exit = false;
		// Signal thread start.
		::SetEvent(thread_start_.get());

		XAMP_ON_SCOPE_EXIT(
			// Stop stream.
			client_->Stop();

			// Signal thread exit.
			::SetEvent(thread_exit_.get());

			// Revert thread priority.
			mmcss.RevertPriority();

			XAMP_LOG_D(logger_, "End exclusive mode stream task!");
		);

		while (!thread_exit && !stop_token.stop_requested()) {
			watch.Reset();

			// Wait for sample ready or stop event.
			auto result = ::WaitForMultipleObjects(static_cast<DWORD>(objects.size()),
				objects.data(), FALSE, current_timeout);

			// Check wait time.
			const auto elapsed = watch.Elapsed<std::chrono::milliseconds>();
			if (elapsed > glitch_time) {
				XAMP_LOG_D(logger_, "WASAPI output got glitch! {}ms > {}ms.", elapsed.count(), glitch_time.count());
				if (!ignore_wait_slow_) {
					thread_exit = true;
					continue;
				}
			}

			switch (result) {
			case WAIT_OBJECT_0 + 0:
				if (!GetSample(false)) {
					thread_exit = true;
				}
				if (!ignore_wait_slow_) {
					current_timeout = static_cast<DWORD>(glitch_time.count());
				}
				break;
			case WAIT_OBJECT_0 + 1:
				thread_exit = true;
				XAMP_LOG_D(logger_, "Stop event trigger!");
				break;
			case WAIT_TIMEOUT:
				XAMP_LOG_D(logger_, "Wait event timeout! {}ms.", elapsed.count());
				thread_exit = true;
				break;
			default:
				XAMP_LOG_D(logger_, "Other error({})!", result);
				thread_exit = true;
				break;
			}
		}
		}, ExecuteFlags::EXECUTE_LONG_RUNNING);

	// Wait thread start.
	constexpr auto kWaitThreadStartSecond = 60 * 1000; // 60sec
	if (::WaitForSingleObject(thread_start_.get(), kWaitThreadStartSecond) == WAIT_TIMEOUT) {
		throw_translated_com_error(HRESULT_FROM_WIN32(ERROR_TIMEOUT));
	}

	// Start stream.
	HrIfFailThrow(client_->Start());
}

void ExclusiveWasapiDevice::SetStreamTime(const double stream_time) noexcept {
	ThrowIf<std::invalid_argument>(mix_format_->nSamplesPerSec != 0,
		"Output sample rate can not set zero.");
	stream_time_ = static_cast<int64_t>(stream_time * static_cast<double>(mix_format_->nSamplesPerSec));
}

double ExclusiveWasapiDevice::GetStreamTime() const noexcept {
	ThrowIf<std::invalid_argument>(mix_format_->nSamplesPerSec != 0,
		"Output sample rate can not set zero.");
    return stream_time_ / static_cast<double>(mix_format_->nSamplesPerSec);
}

uint32_t ExclusiveWasapiDevice::GetVolume() const {
	auto volume_scalar = 0.0F;
	HrIfFailThrow(endpoint_volume_->GetMasterVolumeLevelScalar(&volume_scalar));
	return static_cast<uint32_t>(volume_scalar * 100.0F);
}

void ExclusiveWasapiDevice::SetVolume(uint32_t volume) const {
	// 將音量限制在0~100%之間
	volume = std::clamp(volume, static_cast<uint32_t>(0), static_cast<uint32_t>(100));

	// 如果目前為靜音狀態，先解靜音
	auto is_mute = FALSE;
	HrIfFailThrow(endpoint_volume_->GetMute(&is_mute));
	if (is_mute) {
		HrIfFailThrow(endpoint_volume_->SetMute(FALSE, nullptr));
	}

	// 將百分比轉換為線性比例(0.0f ~ 1.0f)
	float target_volume_scale = static_cast<float>(volume) / 100.0f;

	// 直接以線性比例設定音量
	HrIfFailThrow(endpoint_volume_->SetMasterVolumeLevelScalar(target_volume_scale, nullptr));

	// 若需檢查當前dB值，可呼叫GetMasterVolumeLevel()取得
	float db_volume = 0.0f;
	HrIfFailThrow(endpoint_volume_->GetMasterVolumeLevel(&db_volume));

	XAMP_LOG_D(logger_,
		"Set volume to {}%, linear scale: {:.2f}, current: {:.2f} dB.",
		volume, target_volume_scale, db_volume);
}

void ExclusiveWasapiDevice::SetVolumeLevelScalar(float level) {
	HrIfFailThrow(endpoint_volume_->SetMasterVolumeLevelScalar(level, nullptr));
}

bool ExclusiveWasapiDevice::IsMuted() const {
	auto is_mute = FALSE;
	HrIfFailThrow(endpoint_volume_->GetMute(&is_mute));
	return is_mute;
}

void ExclusiveWasapiDevice::SetMute(const bool mute) const {
	HrIfFailThrow(endpoint_volume_->SetMute(mute, nullptr));
}

PackedFormat ExclusiveWasapiDevice::GetPackedFormat() const noexcept {
    return PackedFormat::INTERLEAVED;
}

uint32_t ExclusiveWasapiDevice::GetBufferSize() const noexcept {
	return buffer_frames_ * mix_format_->nChannels;
}

bool ExclusiveWasapiDevice::IsHardwareControlVolume() const {
	const auto hw_support = (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_VOLUME)
		&& (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_MUTE);
	return hw_support;
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
