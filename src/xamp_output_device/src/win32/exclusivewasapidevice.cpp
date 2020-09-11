#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/logger.h>
#include <base/str_utilts.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/exclusivewasapidevice.h>

namespace xamp::output_device::win32 {

using namespace xamp::output_device::win32::helper;

static void SetWaveformatEx(WAVEFORMATEX *input_fromat, const AudioFormat &audio_format, const int32_t valid_bits_samples) noexcept {
	auto &format = *reinterpret_cast<WAVEFORMATEXTENSIBLE *>(input_fromat);

	format.Format.nChannels = audio_format.GetChannels();
    format.Format.nSamplesPerSec = audio_format.GetSampleRate();
    format.Format.nAvgBytesPerSec = audio_format.GetAvgBytesPerSec();
    format.Format.nBlockAlign = audio_format.GetBlockAlign();

    format.Samples.wValidBitsPerSample = valid_bits_samples;    
    
    if (audio_format.GetChannels() <= 2
            && ((audio_format.GetBitsPerSample() == 16) || (audio_format.GetBitsPerSample() == 8))) {
        format.Format.cbSize = 0;
        format.Format.wFormatTag = WAVE_FORMAT_PCM;
        format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
		format.Format.wBitsPerSample = audio_format.GetBitsPerSample();
    } else {
        format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
		format.Format.wBitsPerSample = 32;
    }
	format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
}

static UINT32 BackwardAligned(const UINT32 bytes_frame, const UINT32 align_size) noexcept {
    return (bytes_frame - (align_size ? (bytes_frame % align_size) : 0));
}

template <typename Predicate>
static int32_t CalcAlignedFramePerBuffer(const UINT32 frames, const UINT32 block_align, Predicate f) noexcept {
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
static int32_t MakeAlignedPeriod(const AudioFormat &format, int32_t frames_per_latency, Predicate f) noexcept {
    return CalcAlignedFramePerBuffer(frames_per_latency, format.GetBlockAlign(), f);
}

static constexpr IID kAudioRenderClientID = __uuidof(IAudioRenderClient);
static constexpr IID kAudioEndpointVolumeCallbackID = __uuidof(IAudioEndpointVolumeCallback);
static constexpr IID kAudioEndpointVolumeID = __uuidof(IAudioEndpointVolume);
static constexpr IID kAudioClient2ID = __uuidof(IAudioClient2);

ExclusiveWasapiDevice::ExclusiveWasapiDevice(CComPtr<IMMDevice> const & device)
	: raw_mode_(false)
	, is_running_(false)
	, is_stop_streaming_(false)
	, thread_priority_(MmcssThreadPriority::MMCSS_THREAD_PRIORITY_NORMAL)
	, buffer_frames_(0)
	, valid_bits_samples_(0)
	, queue_id_(0)
	, stream_time_(0)
	, mmcss_name_(MMCSS_PROFILE_PRO_AUDIO)
	, sample_ready_(nullptr)
	, sample_ready_key_(0)
	, aligned_period_(0)
	, device_(device)
	, callback_(nullptr) {
}

ExclusiveWasapiDevice::~ExclusiveWasapiDevice() {
    try {
        CloseStream();
        sample_ready_.reset();
    } catch (...) {
    }
}

void ExclusiveWasapiDevice::SetAlignedPeriod(REFERENCE_TIME device_period, const AudioFormat &output_format) {
	buffer_frames_ = ReferenceTimeToFrames(device_period, output_format.GetSampleRate());
	buffer_frames_ = MakeAlignedPeriod(output_format, buffer_frames_, BackwardAligned);
	aligned_period_ = MakeHnsPeriod(buffer_frames_, output_format.GetSampleRate());
}

void ExclusiveWasapiDevice::InitialDeviceFormat(const AudioFormat & output_format, const uint32_t valid_bits_samples) {
    HrIfFailledThrow(client_->GetMixFormat(&mix_format_));

	SetWaveformatEx(mix_format_, output_format, valid_bits_samples);

	// Note: 使用IsFormatSupported()某些音效卡格式判斷會不正確, 改用Initialize()進行處理!
    AudioClientProperties device_props{};
    device_props.bIsOffload = FALSE;
    device_props.cbSize = sizeof(device_props);
    device_props.eCategory = AudioCategory_Media;
	device_props.Options = AUDCLNT_STREAMOPTIONS_MATCH_FORMAT | (raw_mode_ ? AUDCLNT_STREAMOPTIONS_RAW : AUDCLNT_STREAMOPTIONS_NONE);;

	try {
		HrIfFailledThrow(client_->SetClientProperties(&device_props));
	}
	catch (Exception const& e) {
		XAMP_LOG_DEBUG("SetClientProperties return failure! {}", e.GetErrorMessage());
		device_props.Options = AUDCLNT_STREAMOPTIONS_MATCH_FORMAT;
		HrIfFailledThrow(client_->SetClientProperties(&device_props));
	}

    REFERENCE_TIME default_device_period = 0;
    REFERENCE_TIME minimum_device_period = 0;
    HrIfFailledThrow(client_->GetDevicePeriod(&default_device_period, &minimum_device_period));

	SetAlignedPeriod(default_device_period, output_format);
	XAMP_LOG_DEBUG("Exclusive mode: default:{} sec, min:{} sec.",
		Nano100ToSeconds(default_device_period),
		Nano100ToSeconds(minimum_device_period));

	XAMP_LOG_DEBUG("WASAPI frame per latency: {}.", buffer_frames_);

	XAMP_LOG_DEBUG("Initial aligned period: {} sec.", Nano100ToSeconds(aligned_period_));

	const auto hr = client_->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		aligned_period_,
		aligned_period_,
		mix_format_,
		nullptr);
	if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
		throw DeviceUnSupportedFormatException(output_format);
	}

	HrIfFailledThrow(hr);
}

void ExclusiveWasapiDevice::OpenStream(const AudioFormat& output_format) {
    stream_time_ = 0;

    auto valid_output_format = output_format;

	constexpr int32_t kValidBitPerSamples = 24;

	// Note: 由於轉換出來就是float格式, 所以固定採用24/32格式進行撥放!
	valid_output_format.SetByteFormat(ByteFormat::SINT32);
	valid_bits_samples_ = kValidBitPerSamples;

	if (!client_) {
		XAMP_LOG_DEBUG("Active device format: {}.", valid_output_format);

        HrIfFailledThrow(device_->Activate(kAudioClient2ID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&client_)));

        HrIfFailledThrow(device_->Activate(kAudioEndpointVolumeID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&endpoint_volume_)));

        InitialDeviceFormat(valid_output_format, valid_bits_samples_);
    }

    HrIfFailledThrow(client_->Reset());
    HrIfFailledThrow(client_->GetService(kAudioRenderClientID,
		reinterpret_cast<void**>(&render_client_)));

    // Enable MCSS
	DWORD task_id = 0;
	queue_id_ = 0;
	HrIfFailledThrow(::MFLockSharedWorkQueue(mmcss_name_.c_str(),
		static_cast<LONG>(thread_priority_),
		&task_id,
		&queue_id_));

	LONG priority = 0;
	HrIfFailledThrow(::MFGetWorkQueueMMCSSPriority(queue_id_, &priority));

	XAMP_LOG_DEBUG("MCSS task id:{} queue id:{}, priority:{} ({}).", task_id, queue_id_, thread_priority_, priority);

    sample_ready_callback_ = new MFAsyncCallback<ExclusiveWasapiDevice>(this,
        &ExclusiveWasapiDevice::OnSampleReady,
		queue_id_);

	HrIfFailledThrow(::MFCreateAsyncResult(nullptr, sample_ready_callback_, nullptr, &sample_ready_async_result_));

	if (!sample_ready_) {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		assert(sample_ready_);
		HrIfFailledThrow(client_->SetEventHandle(sample_ready_.get()));
	}

	vmlock_.UnLock();
	auto buffer_size = buffer_frames_ * output_format.GetChannels();
	buffer_ = MakeBuffer<float>(buffer_size);
	vmlock_.Lock(buffer_.get(), buffer_size * sizeof(float));
    data_convert_ = MakeConvert(output_format, valid_output_format, buffer_frames_);
	XAMP_LOG_DEBUG("WASAPI internal buffer: {}.", FormatBytesBy<float>(buffer_size));

	CComPtr<IAudioEndpointVolume> endpoint_volume;
	HrIfFailledThrow(device_->Activate(kAudioEndpointVolumeID,
		CLSCTX_INPROC_SERVER,
		NULL,
		reinterpret_cast<void**>(&endpoint_volume)
	));

	float min_volume = 0;
	float max_volume = 0;
	float volume_increnment = 0;
	HrIfFailledThrow(endpoint_volume->GetVolumeRange(&min_volume, &max_volume, &volume_increnment));
	XAMP_LOG_DEBUG("WASAPI min_volume: {} max_volume: {} volume_increnment: {}.", min_volume, max_volume, volume_increnment);
}

void ExclusiveWasapiDevice::SetSchedulerService(std::wstring const &mmcss_name, MmcssThreadPriority thread_priority) {
	assert(!mmcss_name.empty());
	thread_priority_ = thread_priority;
	mmcss_name_ = mmcss_name;
}

void ExclusiveWasapiDevice::FillSilentSample(uint32_t frames_available) noexcept {
    BYTE *data = nullptr;
	if (!render_client_) {
		return;
	}
	IgoneAndRaiseError(render_client_->GetBuffer(frames_available, &data));
	IgoneAndRaiseError(render_client_->ReleaseBuffer(frames_available, AUDCLNT_BUFFERFLAGS_SILENT));
}

void ExclusiveWasapiDevice::IgoneAndRaiseError(HRESULT hr) noexcept {
	if (FAILED(hr)) {
		const HRException exception(hr);
		callback_->OnError(exception);
		condition_.notify_one();
		is_running_ = false;
	}	
}

HRESULT ExclusiveWasapiDevice::OnSampleReady(IMFAsyncResult *result) noexcept {
    if (!is_running_) {
        if (!is_stop_streaming_) {
			FillSilentSample(buffer_frames_);
        }
        is_stop_streaming_ = true;
        condition_.notify_all();		
		return S_OK;
	}

	GetSample(buffer_frames_);
    return S_OK;
}

void ExclusiveWasapiDevice::GetSample(uint32_t frame_available) noexcept {
	BYTE* data = nullptr;

	auto stream_time = stream_time_ + frame_available;
	stream_time_ = stream_time;
	stream_time = static_cast<double>(stream_time) / static_cast<double>(mix_format_->nSamplesPerSec);

	auto hr = render_client_->GetBuffer(frame_available, &data);
	if (FAILED(hr)) {
		IgoneAndRaiseError(hr);
		return;
	}

	if (callback_->OnGetSamples(buffer_.get(), frame_available, stream_time) == 0) {
		if (!raw_mode_) {
			(void)DataConverter<InterleavedFormat::INTERLEAVED, InterleavedFormat::INTERLEAVED>::ConvertToInt2432(
				reinterpret_cast<int32_t*>(data),
				buffer_.get(),
				data_convert_);
		}		
		IgoneAndRaiseError(render_client_->ReleaseBuffer(frame_available, 0));
	}
	else {
		IgoneAndRaiseError(render_client_->ReleaseBuffer(frame_available, AUDCLNT_BUFFERFLAGS_SILENT));
		is_running_ = false;
		return;
	}

	IgoneAndRaiseError(::MFPutWaitingWorkItem(sample_ready_.get(),
		0,
		sample_ready_async_result_,
		&sample_ready_key_));
}

void ExclusiveWasapiDevice::AbortStream() noexcept {
	is_stop_streaming_ = true;
}

void ExclusiveWasapiDevice::StopStream(bool wait_for_stop_stream) {
    if (is_running_) {
        is_running_ = false;

        std::unique_lock<std::mutex> lock{ mutex_ };
        while (wait_for_stop_stream && !is_stop_streaming_) {
            condition_.wait(lock);
        }
    }

	::Sleep(aligned_period_ / kWasapiReftimesPerMillisec);

    if (client_ != nullptr) {		
		LogHrFailled(client_->Stop());
		LogHrFailled(client_->Reset());
    }

	::ResetEvent(sample_ready_.get());
}

void ExclusiveWasapiDevice::SetAudioCallback(AudioCallback* callback) noexcept {
	callback_ = callback;
}

bool ExclusiveWasapiDevice::IsStreamOpen() const noexcept {
    return client_ != nullptr;
}

bool ExclusiveWasapiDevice::IsStreamRunning() const noexcept {
    return is_running_;
}

void ExclusiveWasapiDevice::CloseStream() {
    if (queue_id_ != 0) {
		HrIfFailledThrow(::MFUnlockWorkQueue(queue_id_));
		queue_id_ = 0;
	}

	render_client_.Release();
	sample_ready_callback_.Release();
	sample_ready_async_result_.Release();
	callback_ = nullptr;
}

void ExclusiveWasapiDevice::StartStream() {
    if (!client_) {
		throw HRException(AUDCLNT_E_NOT_INITIALIZED);
	}

	assert(callback_ != nullptr);
	
	LogHrFailled(client_->Reset());

	// Note: 必要! 某些音效卡會爆音!
	FillSilentSample(buffer_frames_);

    is_running_ = true;
	HrIfFailledThrow(client_->Start());
	
	HrIfFailledThrow(::MFPutWaitingWorkItem(sample_ready_.get(),
		0,
		sample_ready_async_result_,
		&sample_ready_key_));

    is_stop_streaming_ = false;
}

void ExclusiveWasapiDevice::SetStreamTime(const double stream_time) noexcept {
	stream_time_ = stream_time * static_cast<double>(mix_format_->nSamplesPerSec);
}

double ExclusiveWasapiDevice::GetStreamTime() const noexcept {
    return stream_time_ / static_cast<double>(mix_format_->nSamplesPerSec);
}

uint32_t ExclusiveWasapiDevice::GetVolume() const {
	float channel_volume = 0.0;
	HrIfFailledThrow(endpoint_volume_->GetMasterVolumeLevel(&channel_volume));
	return static_cast<int32_t>(channel_volume);
}

void ExclusiveWasapiDevice::SetVolume(const uint32_t volume) const {
    if (volume > 100) {
		return;
	}

	auto is_mute = FALSE;
	HrIfFailledThrow(endpoint_volume_->GetMute(&is_mute));

	if (is_mute) {
		HrIfFailledThrow(endpoint_volume_->SetMute(false, nullptr));
	}

	const auto channel_volume = volume / 100.0F;
	HrIfFailledThrow(endpoint_volume_->SetMasterVolumeLevelScalar(channel_volume, &GUID_NULL));
}

bool ExclusiveWasapiDevice::IsMuted() const {
	auto is_mute = FALSE;
	HrIfFailledThrow(endpoint_volume_->GetMute(&is_mute));
	return is_mute;
}

void ExclusiveWasapiDevice::SetMute(const bool mute) const {
	HrIfFailledThrow(endpoint_volume_->SetMute(mute, nullptr));
}

void ExclusiveWasapiDevice::DisplayControlPanel() {
}

InterleavedFormat ExclusiveWasapiDevice::GetInterleavedFormat() const noexcept {
    return InterleavedFormat::INTERLEAVED;
}

uint32_t ExclusiveWasapiDevice::GetBufferSize() const noexcept {
	return buffer_frames_ * mix_format_->nChannels;
}

bool ExclusiveWasapiDevice::IsHardwareControlVolume() const {
	CComPtr<IAudioEndpointVolume> endpoint_volume;

	HrIfFailledThrow(device_->Activate(kAudioEndpointVolumeID,
		CLSCTX_INPROC_SERVER,
		nullptr,
		reinterpret_cast<void**>(&endpoint_volume)
	));

	bool hw_support = false;

	DWORD support_mask = 0;
	HrIfFailledThrow(endpoint_volume->QueryHardwareSupport(&support_mask));
	if (support_mask & ENDPOINT_HARDWARE_SUPPORT_VOLUME) {
		XAMP_LOG_DEBUG("Hardware support volume control.");
	}
	else {
		XAMP_LOG_DEBUG("Hardware not support volume control.");
	}
	if (support_mask & ENDPOINT_HARDWARE_SUPPORT_MUTE) {
		XAMP_LOG_DEBUG("Hardware support volume mute.");
	}
	else {
		XAMP_LOG_DEBUG("Hardware not support volume mute.");
	}
	if (support_mask & ENDPOINT_HARDWARE_SUPPORT_METER) {
		XAMP_LOG_DEBUG("Hardware support volume meter.");
	}
	else {
		XAMP_LOG_DEBUG("Hardware not support volume meter.");
	}

	hw_support = (support_mask & ENDPOINT_HARDWARE_SUPPORT_VOLUME)
		&& (support_mask & ENDPOINT_HARDWARE_SUPPORT_MUTE);
	return hw_support;
}

}

#endif
