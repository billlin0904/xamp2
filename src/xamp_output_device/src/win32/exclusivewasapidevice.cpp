#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/scopeguard.h>
#include <base/threadpool.h>
#include <base/logger.h>
#include <base/str_utilts.h>
#include <base/waitabletimer.h>
#include <output_device/win32/mmcss.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/exclusivewasapidevice.h>

namespace xamp::output_device::win32 {

using namespace helper;

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
	, thread_priority_(MmcssThreadPriority::MMCSS_THREAD_PRIORITY_HIGH)
	, buffer_frames_(0)
	, volume_support_mask_(0)
	, stream_time_(0)
	, sample_ready_(nullptr)
	, mmcss_name_(MMCSS_PROFILE_PRO_AUDIO)
	, aligned_period_(0)
	, device_(device)
	, callback_(nullptr)
	, log_(Logger::GetInstance().GetLogger(kExclusiveWasapiDeviceLoggerName)) {
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

#if 0
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
		XAMP_LOG_D(log_, "SetClientProperties return failure! {}", e.GetErrorMessage());
		device_props.Options = AUDCLNT_STREAMOPTIONS_MATCH_FORMAT;
		HrIfFailledThrow(client_->SetClientProperties(&device_props));
	}
#endif

    REFERENCE_TIME default_device_period = 0;
    REFERENCE_TIME minimum_device_period = 0;
    HrIfFailledThrow(client_->GetDevicePeriod(&default_device_period, &minimum_device_period));

	SetAlignedPeriod(default_device_period, output_format);
	XAMP_LOG_D(log_, "Exclusive mode: default:{} sec, min:{} sec.",
		Nano100ToSeconds(default_device_period),
		Nano100ToSeconds(minimum_device_period));

	XAMP_LOG_D(log_, "WASAPI frame per latency: {}.", buffer_frames_);

	XAMP_LOG_D(log_, "Initial aligned period: {} sec.", Nano100ToSeconds(aligned_period_));

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
	
	CComPtr<IAudioEndpointVolume> endpoint_volume;

	HrIfFailledThrow(device_->Activate(kAudioEndpointVolumeID,
		CLSCTX_INPROC_SERVER,
		nullptr,
		reinterpret_cast<void**>(&endpoint_volume)
	));

	HrIfFailledThrow(endpoint_volume->QueryHardwareSupport(&volume_support_mask_));
	
	if (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_VOLUME) {
		XAMP_LOG_D(log_, "Hardware support volume control.");
	}
	else {
		XAMP_LOG_D(log_, "Hardware not support volume control.");
	}
	if (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_MUTE) {
		XAMP_LOG_D(log_, "Hardware support volume mute.");
	}
	else {
		XAMP_LOG_D(log_, "Hardware not support volume mute.");
	}
	if (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_METER) {
		XAMP_LOG_D(log_, "Hardware support volume meter.");
	}
	else {
		XAMP_LOG_D(log_, "Hardware not support volume meter.");
	}
}

void ExclusiveWasapiDevice::OpenStream(const AudioFormat& output_format) {
    stream_time_ = 0;

    auto valid_output_format = output_format;

    // Note: 由於轉換出來就是float格式, 所以固定採用24/32格式進行撥放!
	valid_output_format.SetByteFormat(ByteFormat::SINT32);

	if (!client_) {
		constexpr uint32_t kValidBitPerSamples = 24;
		XAMP_LOG_D(log_, "Active device format: {}.", valid_output_format);

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

	if (!sample_ready_) {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		assert(sample_ready_);
		HrIfFailledThrow(client_->SetEventHandle(sample_ready_.get()));
	}

	if (!thread_start_) {
		thread_start_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		assert(thread_start_);
	}

	if (!thread_exit_) {
		thread_exit_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		assert(thread_exit_);
	}

	if (!close_request_) {
		close_request_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		assert(close_request_);
	}

    const size_t buffer_size = buffer_frames_ * output_format.GetChannels();
	if (buffer_.size() != buffer_size) {
		buffer_ = MakeBuffer<float>(buffer_size);		
	}	
    data_convert_ = MakeConvert(output_format, valid_output_format, buffer_frames_);
	XAMP_LOG_D(log_, "WASAPI internal buffer: {}.", String::FormatBytes(buffer_.GetByteSize()));	
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
	XAMP_LOG_D(log_, "WASAPI min_volume: {} max_volume: {} volume_increnment: {}.", min_volume, max_volume, volume_increment);
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
		DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::ConvertToInt2432(
			reinterpret_cast<int32_t*>(data),
			buffer_.Get(),
			data_convert_);
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
	XAMP_LOG_D(log_, "CloseStream is_running_: {}", is_running_);

	sample_ready_.close();
	thread_start_.close();
	thread_exit_.close();
	close_request_.close();
	render_task_ = std::shared_future<void>();
	render_client_.Release();
	clock_.Release();
}

void ExclusiveWasapiDevice::AbortStream() noexcept {
}

void ExclusiveWasapiDevice::SetIoFormat(DsdIoFormat format) {
	if (format == DsdIoFormat::IO_FORMAT_DSD) {
		raw_mode_ = true;
	} else {
		raw_mode_ = false;
	}
}

DsdIoFormat ExclusiveWasapiDevice::GetIoFormat() const {
	return raw_mode_ ? DsdIoFormat::IO_FORMAT_DSD : DsdIoFormat::IO_FORMAT_PCM;
}

void ExclusiveWasapiDevice::StopStream(bool wait_for_stop_stream) {
	XAMP_LOG_D(log_, "StopStream is_running_: {}", is_running_);
	if (!is_running_) {
		return;
	}

	::SignalObjectAndWait(close_request_.get(), 
		thread_exit_.get(), 
		INFINITE,
		FALSE);

	if (render_task_.valid()) {
		render_task_.get();
	}

	MSleep(std::chrono::milliseconds(100));

	is_running_ = false;
}

void ExclusiveWasapiDevice::StartStream() {
	XAMP_LOG_D(log_, "StartStream!");

	if (!client_) {
		throw HRException(AUDCLNT_E_NOT_INITIALIZED);
	}

	// Note: 必要! 某些音效卡會爆音!
	GetSample(true);

	render_task_ = ThreadPool::WASAPIThreadPool().Spawn([this]() noexcept {
		XAMP_LOG_D(log_, "Start exclusive mode stream task!");

		::SetEvent(thread_start_.get());

		Mmcss mmcss;
		mmcss.BoostPriority(mmcss_name_);

		is_running_ = true;

		const HANDLE objects[2]{ sample_ready_.get(), close_request_.get() };
		auto thread_exit = false;
		while (!thread_exit) {
			auto result = ::WaitForMultipleObjects(2, objects, FALSE, 10 * 1000);
			switch (result) {
			case WAIT_OBJECT_0 + 0:
				GetSample(false);
				break;
			case WAIT_OBJECT_0 + 1:
				thread_exit = true;
				break;
			case WAIT_TIMEOUT:
				XAMP_LOG_D(log_, "Wait event timeout!");
				thread_exit = true;
				break;
			default:
				XAMP_LOG_D(log_, "Other error({})!", result);
				thread_exit = true;
				break;
			}
		}

		client_->Stop();
		::SetEvent(thread_exit_.get());	
		mmcss.RevertPriority();

		XAMP_LOG_D(log_, "End exclusive mode stream task!");
		});

	::WaitForSingleObject(thread_start_.get(), 60 * 1000);
	HrIfFailledThrow(client_->Start());
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
	const auto hw_support = (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_VOLUME)
		&& (volume_support_mask_ & ENDPOINT_HARDWARE_SUPPORT_MUTE);
	return hw_support;
}

}

#endif
