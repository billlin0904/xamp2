#include <base/base.h>
#include <base/assert.h>

#ifdef XAMP_OS_WIN
#include <base/logger.h>
#include <base/waitabletimer.h>
#include <base/ithreadpool.h>
#include <base/dataconverter.h>

#include <output_device/iaudiocallback.h>
#include <output_device/win32/unknownimpl.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/mmcss.h>
#include <output_device/win32/sharedwasapidevice.h>

namespace xamp::output_device::win32 {

using namespace xamp::output_device::win32::helper;

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
	explicit DeviceEventNotification(IAudioCallback* callback) noexcept
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
	IAudioCallback* callback_;
};

SharedWasapiDevice::SharedWasapiDevice(CComPtr<IMMDevice> const & device)
	: is_running_(false)
	, stream_time_(0)
	, buffer_frames_(0)
	, buffer_duration_ms_(0)
	, buffer_time_(0)
	, mmcss_name_(kMmcssProfileProAudio)
	, thread_priority_(MmcssThreadPriority::MMCSS_THREAD_PRIORITY_HIGH)
	, sample_ready_(nullptr)
	, device_(device)
	, callback_(nullptr)
	, log_(Logger::GetInstance().GetLogger(kSharedWasapiDeviceLoggerName)) {
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
	HrIfFailledThrow(device_->Activate(kAudioEndpointVolumeID,
		CLSCTX_INPROC_SERVER,
		nullptr,
		reinterpret_cast<void**>(&endpoint_volume_)
	));
	device_volume_notification_ = new DeviceEventNotification(callback_);
	HrIfFailledThrow(endpoint_volume_->RegisterControlChangeNotify(device_volume_notification_));
}

bool SharedWasapiDevice::IsStreamOpen() const noexcept {
	return render_client_ != nullptr;
}

void SharedWasapiDevice::SetAudioCallback(IAudioCallback* callback) noexcept {
	callback_ = callback;
}

void SharedWasapiDevice::StopStream(bool wait_for_stop_stream) {
	XAMP_LOG_D(log_, "StopStream is_running_: {}", is_running_);
	if (!is_running_) {
		return;
	}

	is_running_ = false;
	rt_work_queue_->Destory();
	client_->Stop();
}

void SharedWasapiDevice::CloseStream() {
	XAMP_LOG_D(log_, "CloseStream is_running_: {}", is_running_);

	UnRegisterDeviceVolumeChange();
	endpoint_volume_.Release();
	device_volume_notification_.Release();

	// We don't close sample ready event immediately,
	// WASAPI can be in same samplerate payback to reset and reuse sample ready event.
	//sample_ready_.close();
	render_client_.Release();
	clock_.Release();
	rt_work_queue_.Release();
}

void SharedWasapiDevice::InitialDeviceFormat(AudioFormat const & output_format) {
	uint32_t fundamental_period_in_frame = 0;
	uint32_t current_period_in_frame = 0;
	uint32_t default_period_in_frame = 0;
	uint32_t max_period_in_frame = 0;
	uint32_t min_period_in_frame = 0;

	mix_format_.Free();
	HrIfFailledThrow(client_->GetCurrentSharedModeEnginePeriod(&mix_format_, &current_period_in_frame));

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

	XAMP_LOG_D(log_, "Initital device format fundamental:{}, current:{}, min:{} max:{}.",
		fundamental_period_in_frame,
		default_period_in_frame,
		min_period_in_frame,
		max_period_in_frame);

	buffer_time_ = default_period_in_frame;

	XAMP_LOG_D(log_, "Use latency: {}", buffer_time_);
}

void SharedWasapiDevice::InitialRawMode(AudioFormat const & output_format) {
	InitialDeviceFormat(output_format);
	HrIfFailledThrow(client_->InitializeSharedAudioStream(AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		buffer_time_,
		mix_format_,
		nullptr));

	BOOL offload_capable = FALSE;
	if (SUCCEEDED(client_->IsOffloadCapable(AudioCategory_Media, &offload_capable))) {
		XAMP_LOG_D(log_, "Devive support offload: {}", offload_capable ? "yes" : "no");
	}
}

void SharedWasapiDevice::OpenStream(AudioFormat const & output_format) {
	stream_time_ = 0;

	if (!client_) {
		XAMP_LOG_D(log_, "Active device format: {}.", output_format);
		
		HrIfFailledThrow(device_->Activate(kAudioClient3ID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&client_)));

		AudioClientProperties device_props{};
		device_props.bIsOffload = FALSE;
		device_props.cbSize = sizeof(device_props);
		device_props.eCategory = AudioCategory_Media;
		device_props.Options = AUDCLNT_STREAMOPTIONS_MATCH_FORMAT | AUDCLNT_STREAMOPTIONS_RAW;
		try {
			HrIfFailledThrow(client_->SetClientProperties(&device_props));
		}
		catch (Exception const& e) {
			XAMP_LOG_D(log_, "SetClientProperties return failure! {}", e.GetErrorMessage());
			device_props.Options = AUDCLNT_STREAMOPTIONS_MATCH_FORMAT;
			HrIfFailledThrow(client_->SetClientProperties(&device_props));
		}

		InitialRawMode(output_format);
	}

	RegisterDeviceVolumeChange();

	LogHrFailled(client_->Reset());

	HrIfFailledThrow(client_->GetBufferSize(&buffer_frames_));
	HrIfFailledThrow(client_->GetService(kAudioRenderClientID, 
		reinterpret_cast<void**>(&render_client_)));
	HrIfFailledThrow(client_->GetService(kAudioClockID,
		reinterpret_cast<void**>(&clock_)));

	XAMP_LOG_D(log_, "WASAPI buffer frame size:{}.", buffer_frames_);

	if (!sample_ready_) {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		XAMP_ASSERT(sample_ready_);
		HrIfFailledThrow(client_->SetEventHandle(sample_ready_.get()));
	}

	rt_work_queue_ = MakeWASAPIWorkQueue(mmcss_name_, this, &SharedWasapiDevice::OnInvoke);
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
	XAMP_ASSERT(!mmcss_name.empty());
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

	const auto channel_volume = static_cast<float>(static_cast<double>(volume) / 100.0);
	HrIfFailledThrow(simple_audio_volume->SetMasterVolume(channel_volume, nullptr));

	XAMP_LOG_D(log_, "Current volume: {}", GetVolume());
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
		is_running_ = false;
	}
}

HRESULT SharedWasapiDevice::GetSample(uint32_t frame_available, bool is_silence) noexcept {
	XAMP_ASSERT(render_client_ != nullptr);
	XAMP_ASSERT(callback_ != nullptr);

	double stream_time = stream_time_ + frame_available;
	stream_time_ = stream_time;
	auto stream_time_float = stream_time / static_cast<double>(mix_format_->nSamplesPerSec);

	DWORD flags = is_silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0;

	BYTE* data = nullptr;
	auto hr = render_client_->GetBuffer(frame_available, &data);
	if (FAILED(hr)) {
		return hr;
	}

	auto sample_time = GetStreamPosInMilliseconds(clock_) / 1000.0;	
	size_t num_filled_frames = 0;
	XAMP_LIKELY(callback_->OnGetSamples(data, frame_available, num_filled_frames, stream_time_float, sample_time) == DataCallbackResult::CONTINUE) {
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
	if (is_running_) {
		try {
			GetSampleRequested(false);
			rt_work_queue_->Queue(sample_ready_.get());
		} catch (Exception const &e) {
			XAMP_LOG_D(log_, e.what());
			StopStream();
		}
	}
	return S_OK;
}

void SharedWasapiDevice::StartStream() {
	XAMP_LOG_D(log_, "StartStream!");

	if (!client_) {
		throw HRException(AUDCLNT_E_NOT_INITIALIZED);
	}

	// Note: 必要! 某些音效卡會爆音!
	GetSampleRequested(true);
	rt_work_queue_->Initial();
	rt_work_queue_->Queue(sample_ready_.get());
	is_running_ = true;
	HrIfFailledThrow(client_->Start());
}

bool SharedWasapiDevice::IsStreamRunning() const noexcept {
	return is_running_;
}

HRESULT SharedWasapiDevice::GetSampleRequested(bool is_silence) noexcept {
	uint32_t padding_frames = 0;

	const auto hr = client_->GetCurrentPadding(&padding_frames);
	if (FAILED(hr)) {
		return hr;
	}

	const auto frames_available = buffer_frames_ - padding_frames;

	if (frames_available > 0) {
		if (is_silence) {
			return GetSample(frames_available, true);
		}
		else {
			return GetSample(frames_available, false);
		}
	}
	return S_OK;
}

void SharedWasapiDevice::AbortStream() noexcept {
}

bool SharedWasapiDevice::IsHardwareControlVolume() const {
	return true;
}

}
#endif
