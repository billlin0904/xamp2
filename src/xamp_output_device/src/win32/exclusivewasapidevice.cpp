#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/logger.h>
#include <base/str_utilts.h>
#include <base/waitabletimer.h>
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
static constexpr IID kAudioEndpointVolumeID = __uuidof(IAudioEndpointVolume);
static constexpr IID kAudioClient2ID = __uuidof(IAudioClient2);
static constexpr IID kAudioClockID = __uuidof(IAudioClock);

ExclusiveWasapiDevice::ExclusiveWasapiDevice(CComPtr<IMMDevice> const & device)
	: raw_mode_(false)
	, is_running_(false)
	, is_stop_require_(false)
	, thread_priority_(MmcssThreadPriority::MMCSS_THREAD_PRIORITY_HIGH)
	, buffer_frames_(0)
	, volume_support_mask_(0)
	, queue_id_(0)
	, stream_time_(0)
	, sample_ready_(nullptr)
	, mmcss_name_(MMCSS_PROFILE_PRO_AUDIO)
	, sample_ready_key_(0)
	, aligned_period_(0)
	, device_(device)
	, callback_(nullptr)
	, log_(Logger::GetInstance().GetLogger("ExclusiveWasapiDevice")) {
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
		XAMP_LOG_I(log_, "SetClientProperties return failure! {}", e.GetErrorMessage());
		device_props.Options = AUDCLNT_STREAMOPTIONS_MATCH_FORMAT;
		HrIfFailledThrow(client_->SetClientProperties(&device_props));
	}

    REFERENCE_TIME default_device_period = 0;
    REFERENCE_TIME minimum_device_period = 0;
    HrIfFailledThrow(client_->GetDevicePeriod(&default_device_period, &minimum_device_period));

	SetAlignedPeriod(default_device_period, output_format);
	XAMP_LOG_I(log_, "Exclusive mode: default:{} sec, min:{} sec.",
		Nano100ToSeconds(default_device_period),
		Nano100ToSeconds(minimum_device_period));

	XAMP_LOG_I(log_, "WASAPI frame per latency: {}.", buffer_frames_);

	XAMP_LOG_I(log_, "Initial aligned period: {} sec.", Nano100ToSeconds(aligned_period_));

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
	
#ifdef _DEBUG
	CComPtr<IAudioEndpointVolume> endpoint_volume;

	HrIfFailledThrow(device_->Activate(kAudioEndpointVolumeID,
		CLSCTX_INPROC_SERVER,
		nullptr,
		reinterpret_cast<void**>(&endpoint_volume)
	));

	HrIfFailledThrow(endpoint_volume->QueryHardwareSupport(&volume_support_mask_));
	
	if (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_VOLUME) {
		XAMP_LOG_I(log_, "Hardware support volume control.");
	}
	else {
		XAMP_LOG_I(log_, "Hardware not support volume control.");
	}
	if (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_MUTE) {
		XAMP_LOG_I(log_, "Hardware support volume mute.");
	}
	else {
		XAMP_LOG_I(log_, "Hardware not support volume mute.");
	}
	if (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_METER) {
		XAMP_LOG_I(log_, "Hardware support volume meter.");
	}
	else {
		XAMP_LOG_I(log_, "Hardware not support volume meter.");
	}
#endif
}

void ExclusiveWasapiDevice::OpenStream(const AudioFormat& output_format) {
    stream_time_ = 0;

    auto valid_output_format = output_format;

	constexpr int32_t kValidBitPerSamples = 24;

	// Note: 由於轉換出來就是float格式, 所以固定採用24/32格式進行撥放!
	valid_output_format.SetByteFormat(ByteFormat::SINT32);

	if (!client_) {
		XAMP_LOG_I(log_, "Active device format: {}.", valid_output_format);

        HrIfFailledThrow(device_->Activate(kAudioClient2ID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&client_)));

        HrIfFailledThrow(device_->Activate(kAudioEndpointVolumeID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&endpoint_volume_)));

        InitialDeviceFormat(valid_output_format, kValidBitPerSamples);
    }

    HrIfFailledThrow(client_->Reset());
    HrIfFailledThrow(client_->GetService(kAudioRenderClientID,
		reinterpret_cast<void**>(&render_client_)));

	HrIfFailledThrow(client_->GetService(kAudioClockID,
		reinterpret_cast<void**>(&clock_)));

	DWORD task_id = 0;
	queue_id_ = 0;
	HrIfFailledThrow(::MFLockSharedWorkQueue(mmcss_name_.c_str(),
		static_cast<LONG>(thread_priority_),
		&task_id,
		&queue_id_));

	LONG priority = 0;
	HrIfFailledThrow(::MFGetWorkQueueMMCSSPriority(queue_id_, &priority));

	XAMP_LOG_I(log_, "MCSS task id:{} queue id:{}, priority:{} ({}).",
		task_id, queue_id_, thread_priority_, EnumToString(static_cast<MmcssThreadPriority>(priority)));

    sample_ready_callback_ = MakeAsyncCallback(this, &ExclusiveWasapiDevice::OnSampleReady, queue_id_);
	HrIfFailledThrow(::MFCreateAsyncResult(nullptr, sample_ready_callback_, nullptr, &sample_ready_async_result_));

	stop_playback_callback_ = MakeAsyncCallback(this, &ExclusiveWasapiDevice::OnStopPlayback, kAsyncCallbackWorkQueue);
	HrIfFailledThrow(::MFCreateAsyncResult(nullptr, stop_playback_callback_, nullptr, &stop_playback_async_result_));

	pause_playback_callback_ = MakeAsyncCallback(this, &ExclusiveWasapiDevice::OnPausePlayback, kAsyncCallbackWorkQueue);
	HrIfFailledThrow(::MFCreateAsyncResult(nullptr, pause_playback_callback_, nullptr, &pause_playback_async_result_));

	start_playback_callback_ = MakeAsyncCallback(this, &ExclusiveWasapiDevice::OnStartPlayback, kAsyncCallbackWorkQueue);
	HrIfFailledThrow(::MFCreateAsyncResult(nullptr, start_playback_callback_, nullptr, &start_playback_async_result_));

	if (!sample_ready_) {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		assert(sample_ready_);
		HrIfFailledThrow(client_->SetEventHandle(sample_ready_.get()));
	}

    const size_t buffer_size = buffer_frames_ * output_format.GetChannels();
	if (buffer_.size() != buffer_size) {
		buffer_ = MakeBuffer<float>(buffer_size);		
	}	
    data_convert_ = MakeConvert(output_format, valid_output_format, buffer_frames_);
	XAMP_LOG_I(log_, "WASAPI internal buffer: {}.", String::FormatBytes(buffer_.GetByteSize()));	
#ifdef _DEBUG
	CComPtr<IAudioEndpointVolume> endpoint_volume;
	HrIfFailledThrow(device_->Activate(kAudioEndpointVolumeID,
		CLSCTX_INPROC_SERVER,
		NULL,
		reinterpret_cast<void**>(&endpoint_volume)
	));
	float min_volume = 0;
	float max_volume = 0;
	float volume_increment = 0;
	HrIfFailledThrow(endpoint_volume->GetVolumeRange(&min_volume, &max_volume, &volume_increment));
	XAMP_LOG_I(log_, "WASAPI min_volume: {} max_volume: {} volume_increnment: {}.", min_volume, max_volume, volume_increment);
#endif
}

void ExclusiveWasapiDevice::SetSchedulerService(std::wstring const &mmcss_name, MmcssThreadPriority thread_priority) {
	assert(!mmcss_name.empty());
	thread_priority_ = thread_priority;
	mmcss_name_ = mmcss_name;
}

void ExclusiveWasapiDevice::ReportError(HRESULT hr) noexcept {
	if (FAILED(hr)) {
		const HRException exception(hr);
		callback_->OnError(exception);
		is_running_ = false;
	}	
}

HRESULT ExclusiveWasapiDevice::OnStartPlayback(IMFAsyncResult* result) {
	// Note: 必要! 某些音效卡會爆音!
	GetSample(true);

	// is_running_必須要確認都成功才能設置為true.
	is_running_ = true;
	is_stop_require_ = false;

	HrIfFailledThrow(client_->Start());

	HrIfFailledThrow(::MFPutWaitingWorkItem(sample_ready_.get(),
		0,
		sample_ready_async_result_,
		&sample_ready_key_));	

	XAMP_LOG_I(log_, "OnStartPlayback");	
	return S_OK;
}

void ExclusiveWasapiDevice::StopStream(bool wait_for_stop_stream) {
	static constexpr std::chrono::milliseconds kTestTimeout{ 10 };
	static constexpr auto kMaxRetryCount = 10;
	
	if (!is_running_) {
		XAMP_LOG_I(log_, "StopStream is_running_: {}", is_running_);
		return;
	}

	XAMP_LOG_I(log_, "StopStream is_running_: {}", is_running_);

	if (wait_for_stop_stream) {
		if (sample_ready_key_ > 0) {
			LogHrFailled(::MFCancelWorkItem(sample_ready_key_));
			XAMP_LOG_I(log_, "Cancel waitting item!");
			sample_ready_key_ = 0;
		}

		GetSample(true);

		is_stop_require_ = true;

		MSleep(800);

		auto i = 0;
		while (is_running_ && i < kMaxRetryCount) {
			std::this_thread::sleep_for(kTestTimeout);
			XAMP_LOG_I(log_, "Wait stop playback callback");
			++i;
		}

		LogHrFailled(client_->Stop());
		XAMP_LOG_I(log_, "OnStopPlayback");
		is_running_ = false;
	}
	else {
		LogHrFailled(client_->Stop());
		XAMP_LOG_I(log_, "OnPausePlayback");
	}
}

HRESULT ExclusiveWasapiDevice::OnPausePlayback(IMFAsyncResult* result) {
	return S_OK;
}
	
HRESULT ExclusiveWasapiDevice::OnStopPlayback(IMFAsyncResult* result) {
	return S_OK;
}

HRESULT ExclusiveWasapiDevice::OnSampleReady(IMFAsyncResult *result) {
	std::lock_guard<FastMutex> render_lock{ render_mutex_ };

	if (is_stop_require_) {
		XAMP_LOG_I(log_, "Device is not running.");
		is_running_ = false;
		return S_OK;
	}
	
	auto hr = GetSample(false);

	if (FAILED(hr)) {
		hr = ::MFPutWorkItem2(kAsyncCallbackWorkQueue,
			0,
			stop_playback_callback_,
			nullptr);
	}
	else {
		hr = ::MFPutWaitingWorkItem(sample_ready_.get(),
			0,
			sample_ready_async_result_,
			&sample_ready_key_);
	}
	return hr;
}

HRESULT ExclusiveWasapiDevice::GetSample(bool is_silence) noexcept {	
	BYTE* data = nullptr;

	auto hr = render_client_->GetBuffer(buffer_frames_, &data);
	if (FAILED(hr)) {
		return hr;
	}
	
	auto stream_time = stream_time_ + buffer_frames_;
	stream_time_ = stream_time;
	auto stream_time_float = static_cast<double>(stream_time) / static_cast<double>(mix_format_->nSamplesPerSec);

	auto sample_time = GetStreamPosInMilliseconds(clock_) / 1000.0;

	const DWORD flags = is_silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0;

	XAMP_LIKELY(callback_->OnGetSamples(buffer_.Get(), buffer_frames_, stream_time_float, sample_time) == DataCallbackResult::CONTINUE) {
		if (!raw_mode_) {
			DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::ConvertToInt2432(
				reinterpret_cast<int32_t*>(data),
				buffer_.Get(),
				data_convert_);
		}
		hr = render_client_->ReleaseBuffer(buffer_frames_, flags);
	}
	else {
		// EOF data
		hr = render_client_->ReleaseBuffer(buffer_frames_, AUDCLNT_BUFFERFLAGS_SILENT);		
	}
	return hr;
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
	start_playback_callback_.Release();
	start_playback_async_result_.Release();
	pause_playback_callback_.Release();
	pause_playback_async_result_.Release();
	stop_playback_callback_.Release();
	stop_playback_async_result_.Release();
	clock_.Release();
}

void ExclusiveWasapiDevice::AbortStream() noexcept {
}

void ExclusiveWasapiDevice::StartStream() {
	if (!client_) {
		throw HRException(AUDCLNT_E_NOT_INITIALIZED);
	}
	
	HrIfFailledThrow(::MFPutWorkItem2(kAsyncCallbackWorkQueue,
		0,
		start_playback_callback_,
		nullptr));

	XAMP_LOG_I(log_, "Start exclusive mode stream!");
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

PackedFormat ExclusiveWasapiDevice::GetPackedFormat() const noexcept {
    return PackedFormat::INTERLEAVED;
}

uint32_t ExclusiveWasapiDevice::GetBufferSize() const noexcept {
	return buffer_frames_ * mix_format_->nChannels;
}

bool ExclusiveWasapiDevice::IsHardwareControlVolume() const {
	auto hw_support = (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_VOLUME)
		&& (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_MUTE);
	return hw_support;
}

}

#endif
