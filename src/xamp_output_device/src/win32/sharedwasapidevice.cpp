#ifdef _WIN32
#include <base/logger.h>
#include <output_device/audiocallback.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/sharedwasapidevice.h>

namespace xamp::output_device::win32 {

constexpr int32_t REFTIMES_PER_MILLISEC = 10000;
constexpr double REFTIMES_PER_SEC = 10000000;

static void SetWaveformatEx(WAVEFORMATEX *input_fromat, const int32_t samplerate) noexcept {
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

SharedWasapiDevice::SharedWasapiDevice(const CComPtr<IMMDevice>& device)
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
        StopStream();
        CloseStream();
        sample_ready_.reset();
    } catch (...) {
    }
}

bool SharedWasapiDevice::IsStreamOpen() const noexcept {
	return client_ != nullptr;
}

void SharedWasapiDevice::SetAudioCallback(AudioCallback* callback) noexcept {
	callback_ = callback;
}

void SharedWasapiDevice::StopStream(bool wait_for_stop_stream) {
	is_stop_streaming_ = false;

	if (is_running_) {
		is_running_ = false;

		std::unique_lock<std::mutex> lock{ mutex_ };
		while (wait_for_stop_stream && !is_stop_streaming_) {
			condition_.wait(lock);
		}
	}

	if (mix_format_ != nullptr) {
		auto sleep_for_stop = REFTIMES_PER_SEC * buffer_frames_ / mix_format_->nSamplesPerSec;
		::Sleep(static_cast<DWORD>(sleep_for_stop / REFTIMES_PER_MILLISEC / 2));
	}

	if (client_ != nullptr) {
		client_->Stop();
	}

	if (sample_raedy_key_ != 0) {
		const auto hr = ::MFCancelWorkItem(sample_raedy_key_);
		if (hr != MF_E_NOT_FOUND) {
			HrIfFailledThrow(hr);
		}
		sample_raedy_key_ = 0;
	}
}

void SharedWasapiDevice::CloseStream() {
	if (queue_id_ != 0) {
		HrIfFailledThrow(::MFUnlockWorkQueue(queue_id_));
		queue_id_ = 0;
	}

	render_client_.Release();
	sample_ready_callback_.Release();
	sample_ready_async_result_.Release();
	callback_ = nullptr;
}

void SharedWasapiDevice::InitialDeviceFormat(const AudioFormat& output_format) {
	uint32_t fundamental_period_in_frame = 0;
	uint32_t current_period_in_frame = 0;
	uint32_t default_period_in_rame = 0;
	uint32_t max_period_in_frame = 0;
	uint32_t min_period_in_frame = 0;

	mix_format_.Free();
	HrIfFailledThrow(client_->GetCurrentSharedModeEnginePeriod(&mix_format_, &current_period_in_frame));
	SetWaveformatEx(mix_format_, output_format.GetSampleRate());

	const auto hr = client_->GetSharedModeEnginePeriod(mix_format_,
		&default_period_in_rame,
		&fundamental_period_in_frame,
		&min_period_in_frame,
		&max_period_in_frame);

	if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
		throw DeviceUnSupportedFormatException();
	}

	XAMP_LOG_DEBUG("Initital device format fundamental:{}, current:{}, min:{} max:{}",
		fundamental_period_in_frame,
		default_period_in_rame,
		min_period_in_frame,
		max_period_in_frame);

	latency_ = default_period_in_rame;

	XAMP_LOG_DEBUG("Use latency: {}", latency_);
}

void SharedWasapiDevice::InitialRawMode(const AudioFormat& output_format) {
	InitialDeviceFormat(output_format);
	HrIfFailledThrow(client_->InitializeSharedAudioStream(AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		latency_,
		mix_format_,
		nullptr));
}

void SharedWasapiDevice::OpenStream(const AudioFormat& output_format) {
	stream_time_ = 0;

	if (!client_) {
		XAMP_LOG_DEBUG("Active device format: {}", output_format);

		HrIfFailledThrow(device_->Activate(__uuidof(IAudioClient3),
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&client_)));

		AudioClientProperties device_props{};
		device_props.bIsOffload = FALSE;
		device_props.cbSize = sizeof(device_props);
		device_props.eCategory = AudioCategory_Media;

		device_props.Options = AUDCLNT_STREAMOPTIONS_RAW | AUDCLNT_STREAMOPTIONS_MATCH_FORMAT;
		if (FAILED(client_->SetClientProperties(&device_props))) {
			HrIfFailledThrow(client_->SetClientProperties(&device_props));
			XAMP_LOG_DEBUG("Device not support RAW mode");
		}
		else {
			XAMP_LOG_DEBUG("Device support RAW mode");
		}

		InitialRawMode(output_format);
	}

	HrIfFailledThrow(client_->Reset());

	HrIfFailledThrow(client_->GetBufferSize(&buffer_frames_));
	HrIfFailledThrow(client_->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&render_client_)));

	XAMP_LOG_DEBUG("Buffer frame size:{}", buffer_frames_);

	// Enable MCSS
	DWORD task_id = 0;
	queue_id_ = 0;
	HrIfFailledThrow(::MFLockSharedWorkQueue(mmcss_name_.c_str(),
		(LONG)thread_priority_
		, &task_id, &queue_id_));

	LONG priority = 0;
	HrIfFailledThrow(::MFGetWorkQueueMMCSSPriority(queue_id_, &priority));

	XAMP_LOG_DEBUG("MCSS task id:{} queue id:{}, priority:{} ({})", task_id, queue_id_, thread_priority_, priority);

	sample_ready_callback_ = new MFAsyncCallback<SharedWasapiDevice>(this,
		&SharedWasapiDevice::OnSampleReady,
		queue_id_);

	HrIfFailledThrow(::MFCreateAsyncResult(nullptr, sample_ready_callback_, nullptr, &sample_ready_async_result_));

	if (!sample_ready_) {
		sample_ready_.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		assert(sample_ready_);
		HrIfFailledThrow(client_->SetEventHandle(sample_ready_.get()));
	}
}

bool SharedWasapiDevice::IsMuted() const {
	CComPtr<ISimpleAudioVolume> simple_audio_volume;
	HrIfFailledThrow(client_->GetService(__uuidof(ISimpleAudioVolume), 
		reinterpret_cast<void**>(&simple_audio_volume)));

	BOOL is_muted = FALSE;
	HrIfFailledThrow(simple_audio_volume->GetMute(&is_muted));
	return is_muted;
}

int32_t SharedWasapiDevice::GetVolume() const {
	CComPtr<ISimpleAudioVolume> simple_audio_volume;
	HrIfFailledThrow(client_->GetService(__uuidof(ISimpleAudioVolume), reinterpret_cast<void**>(&simple_audio_volume)));

	float channel_volume = 0.0;
	HrIfFailledThrow(simple_audio_volume->GetMasterVolume(&channel_volume));
	return static_cast<int32_t>(channel_volume * 100);
}

void SharedWasapiDevice::SetMute(const bool mute) const {
	CComPtr<ISimpleAudioVolume> simple_audio_volume;
	HrIfFailledThrow(client_->GetService(__uuidof(ISimpleAudioVolume), reinterpret_cast<void**>(&simple_audio_volume)));

	HrIfFailledThrow(simple_audio_volume->SetMute(mute, nullptr));
}

void SharedWasapiDevice::DisplayControlPanel() {
}

InterleavedFormat SharedWasapiDevice::GetInterleavedFormat() const noexcept {
	return InterleavedFormat::INTERLEAVED;
}

int32_t SharedWasapiDevice::GetBufferSize() const noexcept {
	return buffer_frames_;
}

void SharedWasapiDevice::SetSchedulerService(const std::wstring& mmcss_name, const MmcssThreadPriority thread_priority) {
	assert(!mmcss_name.empty());
	thread_priority_ = thread_priority;
	mmcss_name_ = mmcss_name;
}

void SharedWasapiDevice::SetVolume(const int32_t volume) const {
	if (volume > 100) {
		return;
	}

	CComPtr<ISimpleAudioVolume> simple_audio_volume;
	HrIfFailledThrow(client_->GetService(__uuidof(ISimpleAudioVolume), reinterpret_cast<void**>(&simple_audio_volume)));

	BOOL is_mute = FALSE;
	HrIfFailledThrow(simple_audio_volume->GetMute(&is_mute));

	if (is_mute) {
		HrIfFailledThrow(simple_audio_volume->SetMute(false, nullptr));
	}

	const auto channel_volume = static_cast<float>(double(volume) / 100.0);
	HrIfFailledThrow(simple_audio_volume->SetMasterVolume(channel_volume, nullptr));
}

void SharedWasapiDevice::SetStreamTime(const double stream_time) noexcept {
	stream_time_ = stream_time * mix_format_->nSamplesPerSec;
}

double SharedWasapiDevice::GetStreamTime() const noexcept {
	return stream_time_ / mix_format_->nSamplesPerSec;
}

void SharedWasapiDevice::GetSample(const int32_t frame_available) {
	stream_time_ = stream_time_ + static_cast<double>(frame_available);

	BYTE* data = nullptr;
	auto hr = render_client_->GetBuffer(frame_available, &data);
	if (FAILED(hr)) {
		const HRException exception(hr);
		callback_->OnError(exception);
		is_running_ = false;
		return;
	}

	if ((*callback_)(reinterpret_cast<float*>(data), frame_available, stream_time_ / mix_format_->nSamplesPerSec) == 0) {
		hr = render_client_->ReleaseBuffer(frame_available, 0);
		if (FAILED(hr)) {
			const HRException exception(hr);
			callback_->OnError(exception);
			is_running_ = false;
		}
	}
	else {
		hr = render_client_->ReleaseBuffer(frame_available, AUDCLNT_BUFFERFLAGS_SILENT);
		if (FAILED(hr)) {
			const HRException exception(hr);
			callback_->OnError(exception);
		}
		is_running_ = false;
	}
}

void SharedWasapiDevice::FillSilentSample(const int32_t frame_available) const {
	BYTE* data;
	HrIfFailledThrow(render_client_->GetBuffer(frame_available, &data));
	HrIfFailledThrow(render_client_->ReleaseBuffer(frame_available, AUDCLNT_BUFFERFLAGS_SILENT));
}

HRESULT SharedWasapiDevice::OnSampleReady(IMFAsyncResult* result) {
	if (!is_running_) {
		if (!is_stop_streaming_) {
			GetSampleRequested(true);
		}
		is_stop_streaming_ = true;
		condition_.notify_all();
		return S_OK;
	}

	GetSampleRequested(false);
	HrIfFailledThrow(::MFPutWaitingWorkItem(sample_ready_.get(), 0, sample_ready_async_result_, &sample_raedy_key_));
	return S_OK;
}

void SharedWasapiDevice::StartStream() {
	if (!client_) {
		throw HRException(AUDCLNT_E_NOT_INITIALIZED);
	}

	assert(callback_ != nullptr);

	client_->Reset();

	is_running_ = true;
	HrIfFailledThrow(client_->Start());
	HrIfFailledThrow(::MFPutWaitingWorkItem(sample_ready_.get(), 0, sample_ready_async_result_, &sample_raedy_key_));
	is_stop_streaming_ = false;
}

bool SharedWasapiDevice::IsStreamRunning() const noexcept {
	return is_running_;
}

void SharedWasapiDevice::GetSampleRequested(const bool is_silence) {
	uint32_t padding_frames = 0;

	const auto hr = client_->GetCurrentPadding(&padding_frames);
	if (FAILED(hr)) {
		const HRException exception(hr);
		callback_->OnError(exception);
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
}

}
#endif
