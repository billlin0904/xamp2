#include <output_device/win32/hrexception.h>
#include <output_device/win32/exclusivewasapidevice.h>

namespace xamp::output_device::win32 {

#define REFTIMES_PER_MILLISEC  10000

static void SetWaveformatEx(WAVEFORMATEX *input_fromat, const AudioFormat &audio_format, const int32_t valid_bits_samples) {
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

static UINT32 BackwardAligned(const UINT32 bytes_frame, const UINT32 align_size) {
    return (bytes_frame - (align_size ? (bytes_frame % align_size) : 0));
}

template <typename Predicate>
static int32_t CalcAlignedFramePerBuffer(const UINT32 frames, const UINT32 block_align, Predicate f) {
    static const auto HD_AUDIO_PACKET_SIZE = 128;

    const auto bytes_frame = frames * block_align;
	auto new_bytes_frame = f(bytes_frame, HD_AUDIO_PACKET_SIZE);
    if (new_bytes_frame < HD_AUDIO_PACKET_SIZE) {
        new_bytes_frame = HD_AUDIO_PACKET_SIZE;
    }

	auto new_frames = new_bytes_frame / block_align;
    const UINT32 packets = new_bytes_frame / HD_AUDIO_PACKET_SIZE;
    new_bytes_frame = packets * HD_AUDIO_PACKET_SIZE;
    new_frames = new_bytes_frame / block_align;
    return new_frames;
}

static double Nano100ToSeconds(REFERENCE_TIME ref) {
	//  1 nano = 0.000000001 seconds
	//100 nano = 0.0000001   seconds
	//100 nano = 0.0001   milliseconds
	return (static_cast<double>(ref) * 0.0000001);
}

static UINT32 ReferenceTimeToFrames(const REFERENCE_TIME period, const UINT32 samplerate) {
    return static_cast<UINT32>(
		1.0 * period * // hns *
        samplerate / // (frames / s) /
        1000 / // (ms / s) /
        10000 // (hns / s) /
        + 0.5 // rounding
    );
}

static REFERENCE_TIME MakeHnsPeriod(const UINT32 frames, const UINT32 samplerate) {
    return static_cast<REFERENCE_TIME>(10000.0 * 1000.0 / double(samplerate) * double(frames) + 0.5);
}

template <typename Predicate>
static int32_t MakeAlignedPeriod(const AudioFormat &format, int32_t frames_per_latency, Predicate f) {
    return CalcAlignedFramePerBuffer(frames_per_latency, format.GetBlockAlign(), f);
}

ExclusiveWasapiDevice::ExclusiveWasapiDevice(const CComPtr<IMMDevice>& device)
	: is_running_(false)
	, is_stop_streaming_(false)
	, thread_priority_(MmcssThreadPriority::MMCSS_THREAD_PRIORITY_HIGH)
	, frames_per_latency_(0)
	, valid_bits_samples_(0)
	, queue_id_(0)
	, stream_time_(0)
	, mmcss_name_(MMCSS_PROFILE_PRO_AUDIO)	
	, sample_ready_(nullptr)
	, sample_raedy_key_(0)
	, aligned_period_(0)
	, device_(device)
	, callback_(nullptr) {
}

ExclusiveWasapiDevice::~ExclusiveWasapiDevice() {
    StopStream();
	CloseStream();
	sample_ready_.reset();
}

void ExclusiveWasapiDevice::SetAlignedPeriod(REFERENCE_TIME device_period, const AudioFormat &output_format) {
	frames_per_latency_ = ReferenceTimeToFrames(device_period, output_format.GetSampleRate());
	frames_per_latency_ = MakeAlignedPeriod(output_format, frames_per_latency_, BackwardAligned);
	aligned_period_ = MakeHnsPeriod(frames_per_latency_, output_format.GetSampleRate());
}

void ExclusiveWasapiDevice::InitialDeviceFormat(const AudioFormat & output_format, const int32_t valid_bits_samples) {
    HR_IF_FAILED_THROW(client_->GetMixFormat(&mix_format_));

	SetWaveformatEx(mix_format_, output_format, valid_bits_samples);

	// Note: 使用IsFormatSupported()某些音效卡格式判斷會不正確, 改用Initialize()進行處理!
    AudioClientProperties device_props{};
    device_props.bIsOffload = FALSE;
    device_props.cbSize = sizeof(device_props);
    device_props.eCategory = AudioCategory_Media;
	device_props.Options = AUDCLNT_STREAMOPTIONS_RAW | AUDCLNT_STREAMOPTIONS_MATCH_FORMAT;

	// Fall back use not raw mode.
	if (FAILED(client_->SetClientProperties(&device_props))) {
		device_props.Options = AUDCLNT_STREAMOPTIONS_MATCH_FORMAT;
		HR_IF_FAILED_THROW(client_->SetClientProperties(&device_props));
		//logger_->debug("Device not support RAW mode");
	} else {
		//logger_->debug("Device support RAW mode");
	}
	
    REFERENCE_TIME default_device_period = 0;
    REFERENCE_TIME minimum_device_period = 0;
    HR_IF_FAILED_THROW(client_->GetDevicePeriod(&default_device_period, &minimum_device_period));

	SetAlignedPeriod(default_device_period, output_format);
	//logger_->debug("Exclusive mode: default:{} sec, min:{} sec",
	//	Nano100ToSeconds(default_device_period),
	//	Nano100ToSeconds(minimum_device_period));

	//logger_->debug("Frame per latency: {}", frames_per_latency_);	

	//logger_->debug("Initial aligned period: {} sec",
    //    Nano100ToSeconds(aligned_period_));

	const auto hr = client_->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		aligned_period_,
		aligned_period_,
		mix_format_,
		nullptr);
	if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
		throw DeviceUnSupportedFormatException();
	}

	HR_IF_FAILED_THROW(hr);
}

void ExclusiveWasapiDevice::OpenStream(const AudioFormat& output_format) {
    stream_time_ = 0;

    auto valid_output_format = output_format;

	// Note: 由於轉換出來就是float格式, 所以固定採用24/32格式進行撥放!
	valid_output_format.SetByteFormat(ByteFormat::SINT32);
	valid_bits_samples_ = 24;

	if (!client_) {
		//logger_->debug("Active device format: {}", valid_output_format.ToString());

        HR_IF_FAILED_THROW(device_->Activate(__uuidof(IAudioClient2),
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&client_)));

        HR_IF_FAILED_THROW(device_->Activate(__uuidof(IAudioEndpointVolume),
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&endpoint_volume_)));

        InitialDeviceFormat(valid_output_format, valid_bits_samples_);
    }

    HR_IF_FAILED_THROW(client_->Reset());
    HR_IF_FAILED_THROW(client_->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&render_client_)));

    // Enable MCSS
	DWORD task_id = 0;
	HR_IF_FAILED_THROW(MFLockSharedWorkQueue(mmcss_name_.c_str(), (LONG)thread_priority_, &task_id, &queue_id_));

	LONG priority = 0;
	HR_IF_FAILED_THROW(MFGetWorkQueueMMCSSPriority(queue_id_, &priority));

	//logger_->debug("MCSS task id:{} queue id:{}, priority:{} ({})", task_id, queue_id_, thread_priority_, priority);

    sample_ready_callback_ = new MFAsyncCallback<ExclusiveWasapiDevice>(this,
        &ExclusiveWasapiDevice::OnSampleReady, queue_id_);

	HR_IF_FAILED_THROW(MFCreateAsyncResult(nullptr, sample_ready_callback_, nullptr, &sample_ready_async_result_));

	if (!sample_ready_) {
		sample_ready_.reset(CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		assert(sample_ready_);
		HR_IF_FAILED_THROW(client_->SetEventHandle(sample_ready_.get()));
	}

	buffer_.reset(new float[frames_per_latency_ * 2]);
    data_covert_ = MakeConvert(output_format, valid_output_format, frames_per_latency_);
}

void ExclusiveWasapiDevice::SetSchedulerService(const std::wstring &mmcss_name, const MmcssThreadPriority thread_priority) {
	assert(!mmcss_name.empty());
	thread_priority_ = thread_priority;
	mmcss_name_ = mmcss_name;
}

void ExclusiveWasapiDevice::FillSilentSample(const int32_t frames_available) const {
    BYTE *data = nullptr;
    HR_IF_FAILED_THROW(render_client_->GetBuffer(frames_available, &data));
    HR_IF_FAILED_THROW(render_client_->ReleaseBuffer(frames_available, AUDCLNT_BUFFERFLAGS_SILENT));
}

HRESULT ExclusiveWasapiDevice::OnSampleReady(IMFAsyncResult *result) {
    BYTE *data = nullptr;

    if (!is_running_) {
        if (!is_stop_streaming_) {
            FillSilentSample(frames_per_latency_);
        }
        is_stop_streaming_ = true;
        condition_.notify_all();
		return S_OK;
	}

    stream_time_ = stream_time_ + static_cast<double>(frames_per_latency_);

	auto hr = render_client_->GetBuffer(frames_per_latency_, &data);
	if (FAILED(hr)) {
		const HRException exception(hr);
		callback_->OnError(exception);
		is_running_ = false;
		return S_OK;
	}

 	if ((*callback_)(buffer_.get(), frames_per_latency_, stream_time_ / mix_format_->nSamplesPerSec) == 0) {
		DataConverter<InterleavedFormat::INTERLEAVED, InterleavedFormat::INTERLEAVED>::Convert2432Bit(
            reinterpret_cast<int32_t*>(data), buffer_.get(), data_covert_);

        hr = render_client_->ReleaseBuffer(frames_per_latency_, 0);
		if (FAILED(hr)) {
			const HRException exception(hr);
			callback_->OnError(exception);
			is_running_ = false;
		}
	} else {
		hr = render_client_->ReleaseBuffer(frames_per_latency_, AUDCLNT_BUFFERFLAGS_SILENT);
		if (FAILED(hr)) {
			const HRException exception(hr);
			callback_->OnError(exception);
		}
		is_running_ = false;
	}

    HR_IF_FAILED_THROW(MFPutWaitingWorkItem(sample_ready_.get(), 0, sample_ready_async_result_, &sample_raedy_key_));
    
    return S_OK;
}

void ExclusiveWasapiDevice::StopStream() {
    is_stop_streaming_ = false;

    if (is_running_) {
        is_running_ = false;

        std::unique_lock<std::mutex> lock{ mutex_ };
        while (!is_stop_streaming_) {
            condition_.wait(lock);
        }
    }

#define REFTIMES_PER_SEC  double(10000000)
#define REFTIMES_PER_MILLISEC  10000

    if (mix_format_ != nullptr) {
        auto sleep_for_stop = REFTIMES_PER_SEC * frames_per_latency_ / mix_format_->nSamplesPerSec;
        Sleep(static_cast<DWORD>(sleep_for_stop / REFTIMES_PER_MILLISEC));
    }    

    if (client_ != nullptr) {
        client_->Stop();
    }

    if (sample_raedy_key_ != 0) {
        HR_IF_FAILED_THROW2(MFCancelWorkItem(sample_raedy_key_), MF_E_NOT_FOUND);
        sample_raedy_key_ = 0;
    }
}

void ExclusiveWasapiDevice::SetAudioCallback(AudioCallback* callback) {
	callback_ = callback;
}

bool ExclusiveWasapiDevice::IsStreamOpen() const {
    return client_ != nullptr;
}

bool ExclusiveWasapiDevice::IsStreamRunning() const {
    return is_running_;
}

void ExclusiveWasapiDevice::CloseStream() {
    if (queue_id_ != 0) {
		HR_IF_FAILED_THROW(MFUnlockWorkQueue(queue_id_));
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
	
    client_->Reset();

    is_running_ = true;
	HR_IF_FAILED_THROW(client_->Start());

	// Note: 必要! 某些音效卡會爆音!
	FillSilentSample(frames_per_latency_);

	HR_IF_FAILED_THROW(MFPutWaitingWorkItem(sample_ready_.get(), 0, sample_ready_async_result_, &sample_raedy_key_));
    is_stop_streaming_ = false;
}

void ExclusiveWasapiDevice::SetStreamTime(const double stream_time) {
    stream_time_ = stream_time * mix_format_->nSamplesPerSec;
}

double ExclusiveWasapiDevice::GetStreamTime() const {
    return stream_time_ / mix_format_->nSamplesPerSec;
}

int32_t ExclusiveWasapiDevice::GetVolume() const {
	float channel_volume = 0.0;
	HR_IF_FAILED_THROW(endpoint_volume_->GetMasterVolumeLevel(&channel_volume));
	return static_cast<int32_t>(channel_volume);
}

void ExclusiveWasapiDevice::SetVolume(const int32_t volume) const {
    if (volume > 100) {
		return;
	}

	auto is_mute = FALSE;
	HR_IF_FAILED_THROW(endpoint_volume_->GetMute(&is_mute));

	if (is_mute) {
		HR_IF_FAILED_THROW(endpoint_volume_->SetMute(false, nullptr));
	}

	const auto channel_volume = static_cast<float>(double(volume) / 100.0);
	HR_IF_FAILED_THROW(endpoint_volume_->SetMasterVolumeLevelScalar(channel_volume, &GUID_NULL));
}

bool ExclusiveWasapiDevice::IsMuted() const {
	auto is_mute = FALSE;
	HR_IF_FAILED_THROW(endpoint_volume_->GetMute(&is_mute));
	return is_mute;
}

void ExclusiveWasapiDevice::SetMute(const bool mute) const {
	HR_IF_FAILED_THROW(endpoint_volume_->SetMute(mute, nullptr));
}

void ExclusiveWasapiDevice::DisplayControlPanel() {
}

InterleavedFormat ExclusiveWasapiDevice::GetInterleavedFormat() const {
    return InterleavedFormat::INTERLEAVED;
}

int32_t ExclusiveWasapiDevice::GetBufferSize() const {
	return frames_per_latency_;
}

}