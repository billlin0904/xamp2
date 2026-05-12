#include <output_device/win32/sharedwasapidevice.h>

#ifdef XAMP_OS_WIN
#include <output_device/iaudiocallback.h>
#include <output_device/win32/unknownimpl.h>
#include <output_device/win32/comexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/mmcss.h>

#include <base/base.h>
#include <base/assert.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/waitabletimer.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

using namespace helper;

namespace {
	/*
	* SetWaveformatEx is a helper function to set WAVEFORMATEX.
	*
	* @param[in] input_format WAVEFORMATEX*
	* @param[in] sample_rate uint32_t
	*/
	void SetWaveformatEx(WAVEFORMATEX* input_format, uint32_t sample_rate) {
		XAMP_EXPECTS(input_format != nullptr);
		XAMP_EXPECTS(input_format->nChannels == AudioFormat::kMaxChannel);
		XAMP_EXPECTS(sample_rate > 0);

		auto& format = *reinterpret_cast<WAVEFORMATEXTENSIBLE*>(input_format);

		format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		format.Format.nChannels = 2;
		format.Format.nBlockAlign = 2 * sizeof(float);
		format.Format.wBitsPerSample = 8 * sizeof(float);
		format.Format.cbSize = 22;
		format.Format.nSamplesPerSec = sample_rate;
		format.Format.nAvgBytesPerSec = format.Format.nSamplesPerSec * format.Format.nBlockAlign;
		format.Samples.wValidBitsPerSample = format.Format.wBitsPerSample;
		format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
		format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
	}

	constexpr auto kSimpleAudioVolumeID = __uuidof(ISimpleAudioVolume);
	constexpr auto kAudioEndpointVolumeCallbackID = __uuidof(IAudioEndpointVolumeCallback);
	constexpr auto kAudioEndpointVolumeID = __uuidof(IAudioEndpointVolume);
	constexpr auto kAudioRenderClientID = __uuidof(IAudioRenderClient);
	constexpr auto kAudioClient3ID = __uuidof(IAudioClient3);
	constexpr auto kAudioClockID = __uuidof(IAudioClock);
	constexpr uint32_t kSharedWasapiLatencyMs = 30;
	constexpr REFERENCE_TIME kHnsPerMillisecond = 10000;

	uint32_t AlignPeriodToFundamental(uint32_t period_in_frames, uint32_t fundamental_period_in_frames) {
		if (fundamental_period_in_frames == 0) {
			return period_in_frames;
		}
		const auto remainder = period_in_frames % fundamental_period_in_frames;
		if (remainder == 0) {
			return period_in_frames;
		}
		return period_in_frames + (fundamental_period_in_frames - remainder);
	}
}

/*
	* DeviceEventNotification is a IAudioEndpointVolumeCallback implementation.
	*/
class SharedWasapiDevice::DeviceEventNotification final
	: public UnknownImpl<IAudioEndpointVolumeCallback> {
public:
	/*
	* Constructor
	*/
	explicit DeviceEventNotification(IAudioCallback* callback) : callback_(callback) {
		XAMP_EXPECTS(callback_ != nullptr);
	}

	/*
	* Destructor
	*/
	virtual ~DeviceEventNotification() override = default;

	/*
	* QueryInterface
	*
	* @param[in] iid REFIID
	* @param[out] ReturnValue void**
	*
	* @return HRESULT
	*/
	HRESULT QueryInterface(REFIID iid, void** return_value) override {
		if (return_value == nullptr) {
			return E_POINTER;
		}
		*return_value = nullptr;
		if (iid == IID_IUnknown) {
			*return_value = static_cast<IUnknown*>(static_cast<IAudioEndpointVolumeCallback*>(this));
			AddRef();
		}
		else if (iid == kAudioEndpointVolumeCallbackID) {
			*return_value = static_cast<IAudioEndpointVolumeCallback*>(this);
			AddRef();
		}
		else {
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	/*
	* OnNotify
	*
	* @param[in] NotificationData PAUDIO_VOLUME_NOTIFICATION_DATA
	*/
	STDMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA notification_data) override {
		callback_->OnVolumeChange(static_cast<int32_t>(notification_data->fMasterVolume * 100.0f));
		return S_OK;
	}

private:
	IAudioCallback* callback_;
};

SharedWasapiDevice::SharedWasapiDevice(bool is_low_latency, CComPtr<IMMDevice> const & device)
	: is_low_latency_(is_low_latency)
	, is_running_(false)
	, stream_time_(0)
	, buffer_frames_(0)
	, buffer_period_in_frames_(0)
	, buffer_duration_hns_(0)
	, thread_priority_(MmcssThreadPriority::MMCSS_THREAD_PRIORITY_HIGH)
	, sample_ready_(nullptr)
	, device_(device)
	, callback_(nullptr)
	, mmcss_name_(kMmcssProfileProAudio)
	, logger_(XampLoggerFactory.GetLogger(XAMP_LOG_NAME(SharedWasapiDevice))) {
}

SharedWasapiDevice::~SharedWasapiDevice() {
    try {
        CloseStream();
        sample_ready_.reset();
    } catch (...) {
    }
	UnRegisterDeviceVolumeChange();
}

void SharedWasapiDevice::UnRegisterDeviceVolumeChange() {
	if (endpoint_volume_ != nullptr) {
		endpoint_volume_->UnregisterControlChangeNotify(device_volume_notification_);
	}
}

void SharedWasapiDevice::RegisterDeviceVolumeChange() {
	HrIfFailThrow(device_->Activate(kAudioEndpointVolumeID,
		CLSCTX_INPROC_SERVER,
		nullptr,
		reinterpret_cast<void**>(&endpoint_volume_)
	));
	device_volume_notification_ = new DeviceEventNotification(callback_);
	HrIfFailThrow(endpoint_volume_->RegisterControlChangeNotify(device_volume_notification_));
}

bool SharedWasapiDevice::IsStreamOpen() const {
	return render_client_ != nullptr;
}

void SharedWasapiDevice::SetAudioCallback(IAudioCallback* callback) {
	callback_ = callback;
}

void SharedWasapiDevice::CloseStream() {
	XAMP_LOG_D(logger_, "CloseStream is_running_: {}", is_running_);

	UnRegisterDeviceVolumeChange();
	endpoint_volume_.Release();
	simple_audio_volume_.Release();
	device_volume_notification_.Release();

	// We don't close sample ready event immediately,
	// WASAPI can be in same sample rate payback to reset and reuse sample ready event.
	//sample_ready_.close();
	render_client_.Release();
	clock_.Release();
	rt_work_queue_.Release();
	client_.Release();
	mix_format_.Free();
}

void SharedWasapiDevice::InitialDeviceFormat(const AudioFormat& output_format) {
	uint32_t fundamental_period_in_frame = 0;
	uint32_t current_period_in_frame = 0;
	uint32_t default_period_in_frame = 0;
	uint32_t max_period_in_frame = 0;
	uint32_t min_period_in_frame = 0;

	mix_format_.Free();

	// Get the mix format and the current shared mode engine period.
	HrIfFailThrow(client_->GetCurrentSharedModeEnginePeriod(&mix_format_, &current_period_in_frame));

	// Set the mix format to the device format.
	SetWaveformatEx(mix_format_, output_format.GetSampleRate());

	// The pFormat parameter below is optional (Its needed only for MATCH_FORMAT clients).
	const auto hr = client_->GetSharedModeEnginePeriod(mix_format_,
		&default_period_in_frame,
		&fundamental_period_in_frame,
		&min_period_in_frame,
		&max_period_in_frame);

	if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
		throw DeviceUnSupportedFormatException(output_format);
	}

	const auto ms_per_samples = 1000.0 / static_cast<double>(output_format.GetSampleRate());

	XAMP_LOG_D(logger_,
		"Initial device format fundamental:{:.2f} msec, current:{:.2f} msec, min:{:.2f} msec max:{:.2f} msec.",
		fundamental_period_in_frame * ms_per_samples,
		default_period_in_frame * ms_per_samples,
		min_period_in_frame * ms_per_samples,
		max_period_in_frame * ms_per_samples);

	const auto requested_period_in_frame = static_cast<uint32_t>(
		(static_cast<uint64_t>(output_format.GetSampleRate()) * kSharedWasapiLatencyMs + 999) / 1000);
	auto period_in_frame = AlignPeriodToFundamental(requested_period_in_frame, fundamental_period_in_frame);
	period_in_frame = (std::max)(period_in_frame, min_period_in_frame);
	if (max_period_in_frame != 0) {
		period_in_frame = (std::min)(period_in_frame, max_period_in_frame);
	}

	buffer_period_in_frames_ = period_in_frame;
	buffer_duration_hns_ = kSharedWasapiLatencyMs * kHnsPerMillisecond;

	XAMP_LOG_D(logger_,
		"Use latency: {:.2f} msec (request:{} msec, period:{} frames).",
		buffer_period_in_frames_ * ms_per_samples,
		kSharedWasapiLatencyMs,
		buffer_period_in_frames_);
}

void SharedWasapiDevice::InitialDevice(const AudioFormat& output_format) {
	if (!is_low_latency_) {
		InitialDeviceFormat(output_format);
		auto hr = client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
			buffer_duration_hns_,
			buffer_duration_hns_,
			mix_format_,
			nullptr);
		if (hr == HRESULT_FROM_WIN32(ERROR_BUSY)) {
			throw DeviceInUseException();
		}
		if (hr == AUDCLNT_E_ENGINE_PERIODICITY_LOCKED) {
			// 會出現這個錯誤, 代表音效設備不支援同時多個 sample rate, 所以需要進行重採樣轉換.			
			throw DeviceNeedSetMatchFormatException();
		}
		HrIfFailThrow(hr);
	} else {
		InitialDeviceFormat(output_format);
		auto hr = client_->InitializeSharedAudioStream(AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
			buffer_period_in_frames_,
			mix_format_,
			nullptr);
		if (hr == HRESULT_FROM_WIN32(ERROR_BUSY)) {
			throw DeviceInUseException();
		}
		HrIfFailThrow(hr);		
	}
}

void SharedWasapiDevice::OpenStream(AudioFormat const & output_format) {
	stream_time_ = 0;

	if (!client_) {
		XAMP_LOG_D(logger_, "Active device format: {}.", output_format);
		
		HrIfFailThrow(device_->Activate(kAudioClient3ID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&client_)));

		// Try to raw mode in client properties.
		AudioClientProperties device_props{};
		device_props.bIsOffload = FALSE;
		device_props.cbSize = sizeof(device_props);
		device_props.eCategory = AudioCategory_Media;
		device_props.Options = AUDCLNT_STREAMOPTIONS_MATCH_FORMAT | AUDCLNT_STREAMOPTIONS_RAW;
		try {
			HrIfFailThrow(client_->SetClientProperties(&device_props));
		}
		catch (const Exception& e) {
			XAMP_LOG_D(logger_, "SetClientProperties return failure! {}", e.GetErrorMessage());
			device_props.Options = AUDCLNT_STREAMOPTIONS_MATCH_FORMAT;
			HrIfFailThrow(client_->SetClientProperties(&device_props));
		}

		InitialDevice(output_format);
	}

	RegisterDeviceVolumeChange();

	// Reset device state.
	HrIfFailThrow(client_->Reset());

	// Get the buffer size.
	HrIfFailThrow(client_->GetBufferSize(&buffer_frames_));

	// Get the render client.
	HrIfFailThrow(client_->GetService(kAudioRenderClientID, 
		reinterpret_cast<void**>(&render_client_)));

	// Get the audio clock.
	HrIfFailThrow(client_->GetService(kAudioClockID,
		reinterpret_cast<void**>(&clock_)));

	XAMP_LOG_D(logger_, "WASAPI buffer frame size:{}.", buffer_frames_);

	// Create sample ready event.
	if (!sample_ready_) {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		XAMP_ASSERT(sample_ready_);		
	}
	HrIfFailThrow(client_->SetEventHandle(sample_ready_.get()));

	// Create the work queue.
	rt_work_queue_ = MakeWasapiWorkQueue(mmcss_name_, this, &SharedWasapiDevice::OnInvoke);

	// Get the device volume interface.
	HrIfFailThrow(client_->GetService(kSimpleAudioVolumeID, reinterpret_cast<void**>(&simple_audio_volume_)));
}

bool SharedWasapiDevice::IsMuted() const {
	BOOL is_muted = FALSE;
	HrIfFailThrow(simple_audio_volume_->GetMute(&is_muted));
	return is_muted;
}

uint32_t SharedWasapiDevice::GetVolume() const {	
	float channel_volume = 0.0;
	HrIfFailThrow(simple_audio_volume_->GetMasterVolume(&channel_volume));
	return static_cast<uint32_t>(channel_volume * 100);
}

void SharedWasapiDevice::SetMute(bool mute) const {
	HrIfFailThrow(simple_audio_volume_->SetMute(mute, nullptr));
}

PackedFormat SharedWasapiDevice::GetPackedFormat() const {
	return PackedFormat::INTERLEAVED;
}

uint32_t SharedWasapiDevice::GetBufferSize() const {
	return buffer_frames_ * mix_format_->nChannels;
}

void SharedWasapiDevice::SetSchedulerService(std::wstring const & mmcss_name, MmcssThreadPriority thread_priority) {
	XAMP_EXPECTS(!mmcss_name.empty());
	thread_priority_ = thread_priority;
	mmcss_name_ = mmcss_name;
}

void SharedWasapiDevice::SetVolume(uint32_t volume) const {
	volume = std::clamp(volume, static_cast<uint32_t>(0), static_cast<uint32_t>(100));

	BOOL is_mute = FALSE;
	HrIfFailThrow(simple_audio_volume_->GetMute(&is_mute));

	if (is_mute) {
		HrIfFailThrow(simple_audio_volume_->SetMute(false, nullptr));
	}

	const auto channel_volume = static_cast<float>(static_cast<double>(volume) / 100.0);
	HrIfFailThrow(simple_audio_volume_->SetMasterVolume(channel_volume, nullptr));

	XAMP_LOG_D(logger_, "Current volume: {}", GetVolume());
}

void SharedWasapiDevice::SetStreamTime(double stream_time) {
	stream_time_.store(static_cast<int64_t>(
		stream_time * static_cast<double>(mix_format_->nSamplesPerSec)),
		std::memory_order_relaxed);
}

double SharedWasapiDevice::GetStreamTime() const {
	return static_cast<double>(stream_time_.load(std::memory_order_relaxed))
		/ static_cast<double>(mix_format_->nSamplesPerSec);
}

void SharedWasapiDevice::ReportError(HRESULT hr) {
	if (FAILED(hr)) {
		callback_->OnError(com_to_system_error(hr));
		is_running_ = false;
	}
}

HRESULT SharedWasapiDevice::GetSample(uint32_t frame_available, bool is_silence) {
	XAMP_EXPECTS(render_client_ != nullptr);
	XAMP_EXPECTS(callback_ != nullptr);

	// Calculate stream time.
	const auto stream_time = stream_time_.load(std::memory_order_relaxed)
		+ static_cast<int64_t>(frame_available);
	stream_time_.store(stream_time, std::memory_order_relaxed);
	const auto stream_time_float = static_cast<double>(stream_time)
		/ static_cast<double>(mix_format_->nSamplesPerSec);

	DWORD flags = is_silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0;

	BYTE* data = nullptr;
	auto hr = render_client_->GetBuffer(frame_available, &data);
	if (FAILED(hr)) {
		return hr;
	}

	// Calculate sample time.
	const auto sample_time = GetStreamPosInMilliseconds(clock_) / 1000.0;	

	size_t num_filled_frames = 0;

	// Get sample from callback.
	const auto callback_result = callback_->OnGetSamples(data, frame_available, num_filled_frames, stream_time_float, sample_time);
	if (callback_result == DataCallbackResult::CONTINUE) {
		if (num_filled_frames != frame_available) {
			flags = AUDCLNT_BUFFERFLAGS_SILENT;
		}				
		hr = render_client_->ReleaseBuffer(frame_available, flags);
	}
	else {		
		hr = render_client_->ReleaseBuffer(frame_available, AUDCLNT_BUFFERFLAGS_SILENT);
	}
	return hr;
}

HRESULT SharedWasapiDevice::OnInvoke(IMFAsyncResult *) {
	if (is_running_ && rt_work_queue_ != nullptr) {
		if (!is_playing_) {
			is_playing_ = true;
			wait_for_start_stream_cond_.notify_one();
		}

		try {
			GetSample(false);
			rt_work_queue_->WaitAsync(sample_ready_.get());
		} catch (const std::exception &e) {
			XAMP_LOG_D(logger_, e.what());
			if (callback_ != nullptr) {
				callback_->OnError(e);
			}
			is_running_ = false;
			if (client_) {
				client_->Stop();
			}
		}
	}
	return S_OK;
}

void SharedWasapiDevice::StopStream(bool wait_for_stop_stream) {
	std::unique_lock<FastMutex> guard{ mutex_ };

	XAMP_LOG_D(logger_, "StopStream is_running_: {}", is_running_);
	if (!is_running_) {
		return;
	}

	is_running_ = false;
	rt_work_queue_->Destroy();
	HrIfFailThrow(client_->Stop());
}

void SharedWasapiDevice::StartStream() {
	std::unique_lock<FastMutex> guard{ mutex_ };

	XAMP_LOG_D(logger_, "StartStream!");

	if (!client_) {
		throw_translated_com_error(AUDCLNT_E_NOT_INITIALIZED);
	}

	is_playing_ = false;
	// Note: 必要! 某些音效卡會爆音!
	GetSample(true);
	rt_work_queue_->LoadStream();
	rt_work_queue_->WaitAsync(sample_ready_.get());
	is_running_ = true;
	HrIfFailThrow(client_->Start());

	while (!is_playing_) {
		if (wait_for_start_stream_cond_.wait_for(guard, kWaitStreamStartTimeout) == std::cv_status::timeout) {
			XAMP_LOG_E(logger_, "SharedWasapiDevice start render timeout.");
			return;
		}
	}
}

bool SharedWasapiDevice::IsStreamRunning() const {
	return is_running_;
}

HRESULT SharedWasapiDevice::GetSample(bool is_silence) {
	uint32_t padding_frames = 0;

	const auto hr = client_->GetCurrentPadding(&padding_frames);
	if (FAILED(hr)) {
		XAMP_LOG_DEBUG("Failure");
		return hr;
	}

	const auto frames_available = buffer_frames_ - padding_frames;

	if (frames_available > 0) {
		XAMP_LOG_DEBUG("Get {} samples ({}).", frames_available, padding_frames);
		if (is_silence) {
			return GetSample(frames_available, true);
		}
		else {
			return GetSample(frames_available, false);
		}
	}
	XAMP_LOG_DEBUG("frames_available = 0");
	return S_OK;
}

void SharedWasapiDevice::AbortStream() {
	is_running_ = false;
}

bool SharedWasapiDevice::IsHardwareControlVolume() const {
	return false;
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
