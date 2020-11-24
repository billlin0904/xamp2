#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/logger.h>

#include <output_device/audiocallback.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/sharedwasapidevice.h>

namespace xamp::output_device::win32 {

static void SetWaveformatEx(WAVEFORMATEX *input_fromat, uint32_t samplerate) noexcept {
	auto &format = *reinterpret_cast<WAVEFORMATEXTENSIBLE *>(input_fromat);

	format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	format.Format.nChannels = 2;
	format.Format.nBlockAlign = 2 * sizeof(float);
	format.Format.wBitsPerSample = 8 * sizeof(float);
	format.Format.cbSize = 22;
	format.Samples.wValidBitsPerSample = format.Format.wBitsPerSample;
	format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
	format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
	format.Format.nSamplesPerSec = samplerate;
	format.Format.nAvgBytesPerSec = format.Format.nSamplesPerSec * format.Format.nBlockAlign;
}

static constexpr IID kSimpleAudioVolumeID = __uuidof(ISimpleAudioVolume);
static constexpr IID kAudioEndpointVolumeCallbackID = __uuidof(IAudioEndpointVolumeCallback);
static constexpr IID kAudioEndpointVolumeID = __uuidof(IAudioEndpointVolume);
static constexpr IID kAudioRenderClientID =__uuidof(IAudioRenderClient);
static constexpr IID kAudioClient3ID = __uuidof(IAudioClient3);
static constexpr IID kAudioClockID = __uuidof(IAudioClock);

class SharedWasapiDevice::DeviceEventNotification final
	: public UnknownImpl<IAudioEndpointVolumeCallback> {
public:
	explicit DeviceEventNotification(AudioCallback* callback) noexcept
		: callback_(callback) {
	}

	HRESULT QueryInterface(REFIID iid, void** ReturnValue) override {		
		if (ReturnValue == nullptr) {
			return E_POINTER;
		}
		*ReturnValue = nullptr;
		if (iid == IID_IUnknown) {
			*ReturnValue = static_cast<IUnknown*>(static_cast<IAudioEndpointVolumeCallback*>(this));
			AddRef();
		}
		else if (iid == kAudioEndpointVolumeCallbackID) {
			*ReturnValue = static_cast<IAudioEndpointVolumeCallback*>(this);
			AddRef();
		}
		else {
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	STDMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA NotificationData) override {
		callback_->OnVolumeChange(NotificationData->fMasterVolume);
		return S_OK;
	}

private:
	AudioCallback* callback_;
};

SharedWasapiDevice::SharedWasapiDevice(CComPtr<IMMDevice> const & device)
	: is_running_(false)
	, is_stop_streaming_(false)
	, stream_time_(0)
	, queue_id_(0)
	, latency_(0)
	, buffer_frames_(0)
	, mmcss_name_(MMCSS_PROFILE_PRO_AUDIO)
	, thread_priority_(MmcssThreadPriority::MMCSS_THREAD_PRIORITY_HIGH)
    , sample_raedy_key_(0)
	, sample_ready_(nullptr)
	, device_(device)
	, callback_(nullptr) {
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
}

void SharedWasapiDevice::RegisterDeviceVolumeChange() {
	CComPtr<IAudioEndpointVolume> endpoint_volume;
	HrIfFailledThrow(device_->Activate(kAudioEndpointVolumeID,
		CLSCTX_INPROC_SERVER,
		nullptr,
		reinterpret_cast<void**>(&endpoint_volume)
	));
	device_volume_notification_ = new DeviceEventNotification(callback_);
	HrIfFailledThrow(endpoint_volume->RegisterControlChangeNotify(device_volume_notification_));
}

bool SharedWasapiDevice::IsStreamOpen() const noexcept {
	return render_client_ != nullptr;
}

void SharedWasapiDevice::SetAudioCallback(AudioCallback* callback) noexcept {
	callback_ = callback;
}

void SharedWasapiDevice::StopStream(bool wait_for_stop_stream) {
	if (is_running_) {
		is_running_ = false;

		std::unique_lock<std::mutex> lock{ mutex_ };
		while (wait_for_stop_stream && !is_stop_streaming_) {
			condition_.wait(lock);
		}
	}

	if (client_ != nullptr) {
		LogHrFailled(client_->Stop());
		LogHrFailled(client_->Reset());
	}

	::ResetEvent(sample_ready_.get());
}

void SharedWasapiDevice::CloseStream() {
	if (queue_id_ != 0) {
		HrIfFailledThrow(::MFUnlockWorkQueue(queue_id_));
		queue_id_ = 0;
	}

	render_client_.Release();
	clock_.Release();
	
	sample_ready_callback_.Release();
	sample_ready_async_result_.Release();

	callback_ = nullptr;
}

void SharedWasapiDevice::InitialDeviceFormat(AudioFormat const & output_format) {
	uint32_t fundamental_period_in_frame = 0;
	uint32_t current_period_in_frame = 0;
	uint32_t default_period_in_rame = 0;
	uint32_t max_period_in_frame = 0;
	uint32_t min_period_in_frame = 0;

	mix_format_.Free();
	HrIfFailledThrow(client_->GetCurrentSharedModeEnginePeriod(&mix_format_, &current_period_in_frame));

	SetWaveformatEx(mix_format_, output_format.GetSampleRate());

	// The pFormat parameter below is optional (Its needed only for MATCH_FORMAT clients).
	const auto hr = client_->GetSharedModeEnginePeriod(mix_format_,
		&default_period_in_rame,
		&fundamental_period_in_frame,
		&min_period_in_frame,
		&max_period_in_frame);

	if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
		throw DeviceUnSupportedFormatException(output_format);
	}	

	XAMP_LOG_DEBUG("Initital device format fundamental:{}, current:{}, min:{} max:{}.",
		fundamental_period_in_frame,
		default_period_in_rame,
		min_period_in_frame,
		max_period_in_frame);

	latency_ = default_period_in_rame;

	XAMP_LOG_DEBUG("Use latency: {}", latency_);
}

void SharedWasapiDevice::InitialRawMode(AudioFormat const & output_format) {
	InitialDeviceFormat(output_format);
	HrIfFailledThrow(client_->InitializeSharedAudioStream(AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		latency_,
		mix_format_,
		nullptr));
}

void SharedWasapiDevice::OpenStream(AudioFormat const & output_format) {
	stream_time_ = 0;

	if (!client_) {
		XAMP_LOG_DEBUG("Active device format: {}.", output_format);
		
		HrIfFailledThrow(device_->Activate(kAudioClient3ID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&client_)));

		AudioClientProperties device_props{};
		device_props.bIsOffload = FALSE;
		device_props.cbSize = sizeof(device_props);
		device_props.eCategory = AudioCategory_Media;
		device_props.Options = AUDCLNT_STREAMOPTIONS_MATCH_FORMAT;
		HrIfFailledThrow(client_->SetClientProperties(&device_props));
		InitialRawMode(output_format);
		RegisterDeviceVolumeChange();
	}

	LogHrFailled(client_->Reset());

	HrIfFailledThrow(client_->GetBufferSize(&buffer_frames_));
	HrIfFailledThrow(client_->GetService(kAudioRenderClientID, 
		reinterpret_cast<void**>(&render_client_)));
	HrIfFailledThrow(client_->GetService(kAudioClockID,
		reinterpret_cast<void**>(&clock_)));

	XAMP_LOG_DEBUG("WASAPI buffer frame size:{}.", buffer_frames_);

	// Enable MCSS
	DWORD task_id = 0;
	queue_id_ = 0;
	HrIfFailledThrow(::MFLockSharedWorkQueue(mmcss_name_.c_str(),
		static_cast<LONG>(thread_priority_),
		&task_id,
		&queue_id_));

	LONG priority = 0;
	HrIfFailledThrow(::MFGetWorkQueueMMCSSPriority(queue_id_, &priority));

	XAMP_LOG_DEBUG("MCSS task id:{} queue id:{}, priority:{} ({}).",
		task_id, queue_id_, thread_priority_, (MmcssThreadPriority)priority);

	sample_ready_callback_.Release();
	sample_ready_async_result_.Release();

	sample_ready_callback_ = new MFAsyncCallback<SharedWasapiDevice>(this,
		&SharedWasapiDevice::OnSampleReady,
		queue_id_);

	HrIfFailledThrow(::MFCreateAsyncResult(nullptr, 
		sample_ready_callback_,
		nullptr,
		&sample_ready_async_result_));

	if (!sample_ready_) {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		assert(sample_ready_);
		HrIfFailledThrow(client_->SetEventHandle(sample_ready_.get()));
	}
}

bool SharedWasapiDevice::IsMuted() const {
	CComPtr<ISimpleAudioVolume> simple_audio_volume;
	HrIfFailledThrow(client_->GetService(kSimpleAudioVolumeID,
		reinterpret_cast<void**>(&simple_audio_volume)));

	BOOL is_muted = FALSE;
	HrIfFailledThrow(simple_audio_volume->GetMute(&is_muted));
	return is_muted;
}

uint32_t SharedWasapiDevice::GetVolume() const {
	CComPtr<ISimpleAudioVolume> simple_audio_volume;
	HrIfFailledThrow(client_->GetService(kSimpleAudioVolumeID, reinterpret_cast<void**>(&simple_audio_volume)));

	float channel_volume = 0.0;
	HrIfFailledThrow(simple_audio_volume->GetMasterVolume(&channel_volume));
	return static_cast<uint32_t>(channel_volume * 100);
}

void SharedWasapiDevice::SetMute(bool mute) const {
	CComPtr<ISimpleAudioVolume> simple_audio_volume;
	HrIfFailledThrow(client_->GetService(kSimpleAudioVolumeID, reinterpret_cast<void**>(&simple_audio_volume)));

	HrIfFailledThrow(simple_audio_volume->SetMute(mute, nullptr));
}

void SharedWasapiDevice::DisplayControlPanel() {
}

PackedFormat SharedWasapiDevice::GetPackedFormat() const noexcept {
	return PackedFormat::INTERLEAVED;
}

uint32_t SharedWasapiDevice::GetBufferSize() const noexcept {
	return buffer_frames_;
}

void SharedWasapiDevice::SetSchedulerService(std::wstring const & mmcss_name, MmcssThreadPriority thread_priority) {
	assert(!mmcss_name.empty());
	thread_priority_ = thread_priority;
	mmcss_name_ = mmcss_name;
}

void SharedWasapiDevice::SetVolume(uint32_t volume) const {
	if (volume > 100) {
		return;
	}

	CComPtr<ISimpleAudioVolume> simple_audio_volume;	
	HrIfFailledThrow(client_->GetService(kSimpleAudioVolumeID, reinterpret_cast<void**>(&simple_audio_volume)));

	BOOL is_mute = FALSE;
	HrIfFailledThrow(simple_audio_volume->GetMute(&is_mute));

	if (is_mute) {
		HrIfFailledThrow(simple_audio_volume->SetMute(false, nullptr));
	}

	const auto channel_volume = static_cast<float>(double(volume) / 100.0);
	HrIfFailledThrow(simple_audio_volume->SetMasterVolume(channel_volume, nullptr));
}

void SharedWasapiDevice::SetStreamTime(double stream_time) noexcept {
	stream_time_ = stream_time * static_cast<double>(mix_format_->nSamplesPerSec);
}

double SharedWasapiDevice::GetStreamTime() const noexcept {
	return stream_time_ / static_cast<double>(mix_format_->nSamplesPerSec);
}

void SharedWasapiDevice::ReportError(HRESULT hr) {
	if (FAILED(hr)) {
		const HRException exception(hr);
		callback_->OnError(exception);
		condition_.notify_one();
		is_running_ = false;
	}
}

void SharedWasapiDevice::GetSample(uint32_t frame_available) noexcept {
	double stream_time = stream_time_ + frame_available;
	stream_time_ = stream_time;
	auto stream_time_float = static_cast<double>(stream_time) / static_cast<double>(mix_format_->nSamplesPerSec);

	BYTE* data = nullptr;
	auto hr = render_client_->GetBuffer(frame_available, &data);
	if (FAILED(hr)) {
		ReportError(hr);
		return;
	}

	auto sample_time = helper::GetStreamPosInMilliseconds(clock_) / 1000.0;

	if (callback_->OnGetSamples(reinterpret_cast<float*>(data), frame_available, stream_time_float, sample_time) == 0) {
		ReportError(render_client_->ReleaseBuffer(frame_available, 0));
	}
	else {
		ReportError(render_client_->ReleaseBuffer(frame_available, AUDCLNT_BUFFERFLAGS_SILENT));
		is_running_ = false;
	}
}

void SharedWasapiDevice::FillSilentSample(uint32_t frame_available) noexcept {
	try {
		BYTE* data;
		ReportError(render_client_->GetBuffer(frame_available, &data));
		ReportError(render_client_->ReleaseBuffer(frame_available, AUDCLNT_BUFFERFLAGS_SILENT));
	}
	catch (...) {
		ReportError(E_POINTER);
	}
}

HRESULT SharedWasapiDevice::OnSampleReady(IMFAsyncResult* result) noexcept {
	if (!is_running_) {
		if (!is_stop_streaming_) {
			GetSampleRequested(true);
		}
		is_stop_streaming_ = true;
		condition_.notify_all();
		return S_OK;
	}

	GetSampleRequested(false);	
	return S_OK;
}

void SharedWasapiDevice::StartStream() {
	if (!client_) {
		throw HRException(AUDCLNT_E_NOT_INITIALIZED);
	}

	assert(callback_ != nullptr);

	LogHrFailled(client_->Reset());

	// Note: 必要! 某些音效卡會爆音!
	FillSilentSample(buffer_frames_);

	is_running_ = true;
	HrIfFailledThrow(client_->Start());

	HrIfFailledThrow(::MFPutWaitingWorkItem(sample_ready_.get(), 0, sample_ready_async_result_, &sample_raedy_key_));

	is_stop_streaming_ = false;

	XAMP_LOG_DEBUG("Start shared mode stream!");
}

bool SharedWasapiDevice::IsStreamRunning() const noexcept {
	return is_running_;
}

void SharedWasapiDevice::GetSampleRequested(bool is_silence) noexcept {
	uint32_t padding_frames = 0;

	const auto hr = client_->GetCurrentPadding(&padding_frames);
	if (FAILED(hr)) {
		ReportError(hr);
		return;
	}

	const auto frames_available = buffer_frames_ - padding_frames;

	if (frames_available > 0) {
		if (is_silence) {
			FillSilentSample(frames_available);
		}
		else {
			GetSample(frames_available);
		}
	}
	ReportError(::MFPutWaitingWorkItem(sample_ready_.get(), 0, sample_ready_async_result_, &sample_raedy_key_));
}

void SharedWasapiDevice::AbortStream() noexcept {
	is_stop_streaming_ = true;
}

bool SharedWasapiDevice::IsHardwareControlVolume() const {
	return true;
}

}
#endif
