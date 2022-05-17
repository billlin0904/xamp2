#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/ithreadpool.h>
#include <base/logger.h>
#include <base/str_utilts.h>
#include <base/waitabletimer.h>
#include <base/stopwatch.h>

#include <output_device/iaudiocallback.h>
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
	SetWaveformatEx(mix_format_, output_format, valid_bits_samples);

	REFERENCE_TIME default_device_period = 0;
	REFERENCE_TIME minimum_device_period = 0;

	if (buffer_period_ == 0) {		
		HrIfFailledThrow(client_->GetDevicePeriod(&default_device_period, &minimum_device_period));
	} else {
		default_device_period = buffer_period_;
	}

	SetAlignedPeriod(default_device_period, output_format);

	XAMP_LOG_D(log_, "Device period: default: {:.2f} msec, min: {:.2f} msec.",
		Nano100ToMillis(default_device_period),
		Nano100ToMillis(minimum_device_period));
	XAMP_LOG_D(log_, "Initial aligned period: {:.2f} msec, buffer frames: {}.",
		Nano100ToMillis(aligned_period_), 
		GetBufferSize());

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
		HrIfFailledThrow(hr);
	}
	
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
		XAMP_LOG_D(log_, "Active device format: {}.", valid_output_format);

        HrIfFailledThrow(device_->Activate(kAudioClient2ID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&client_)));

        HrIfFailledThrow(device_->Activate(kAudioEndpointVolumeID,
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&endpoint_volume_)));

		HrIfFailledThrow(client_->GetMixFormat(&mix_format_));

		HRESULT hr = S_OK;
		hr = client_->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, mix_format_, nullptr);
		// note: 某些DAC driver不支援24/32 format, 如果出現AUDCLNT_E_UNSUPPORTED_FORMAT嘗試改用32.
		// 不管是24/32或是32/32 format資料都是24/32.
		if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
			InitialDeviceFormat(valid_output_format, 32);
			XAMP_LOG_D(log_, "Fallback use valid output format: 32.");
		} else {
			constexpr uint32_t kValidBitPerSamples = 24;
			InitialDeviceFormat(valid_output_format, kValidBitPerSamples);
			XAMP_LOG_D(log_, "Use valid output format: {}.", kValidBitPerSamples);
		}
    }

    HrIfFailledThrow(client_->Reset());
    HrIfFailledThrow(client_->GetService(kAudioRenderClientID,
		reinterpret_cast<void**>(&render_client_)));

	HrIfFailledThrow(client_->GetService(kAudioClockID,
		reinterpret_cast<void**>(&clock_)));

	if (!sample_ready_) {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		HrIfFailledThrow(client_->SetEventHandle(sample_ready_.get()));
	}

	if (!thread_start_) {
		thread_start_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
	}

	if (!thread_exit_) {
		thread_exit_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
	}

	if (!close_request_) {
		close_request_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
	}

    const size_t buffer_size = buffer_frames_ * output_format.GetChannels();
	if (buffer_.size() != buffer_size) {
		buffer_ = MakeBuffer<float>(buffer_size);		
	}	
    data_convert_ = MakeConvert(output_format, valid_output_format, buffer_frames_);
	XAMP_LOG_D(log_, "WASAPI internal buffer: {}.", String::FormatBytes(buffer_.GetByteSize()));
}

void ExclusiveWasapiDevice::SetSchedulerService(std::wstring const &mmcss_name, MmcssThreadPriority thread_priority) {
	XAMP_ASSERT(!mmcss_name.empty());
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

bool ExclusiveWasapiDevice::GetSample(bool is_silence) noexcept {
	XAMP_ASSERT(render_client_ != nullptr);
	XAMP_ASSERT(callback_ != nullptr);

	BYTE* data = nullptr;

	auto hr = render_client_->GetBuffer(buffer_frames_, &data);
	if (FAILED(hr)) {
		return false;
	}
	
	auto stream_time = stream_time_ + buffer_frames_;
	stream_time_ = stream_time;
	auto stream_time_float = static_cast<double>(stream_time) / static_cast<double>(mix_format_->nSamplesPerSec);

	auto sample_time = GetStreamPosInMilliseconds(clock_) / 1000.0;

	DWORD flags = is_silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0;

	size_t num_filled_frames = 0;
	XAMP_LIKELY(callback_->OnGetSamples(buffer_.Get(), buffer_frames_, num_filled_frames, stream_time_float, sample_time) == DataCallbackResult::CONTINUE) {
		bool result = true;
		if (num_filled_frames != buffer_frames_) {
			flags = AUDCLNT_BUFFERFLAGS_SILENT;
			result = false;
		}
		DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::ConvertToInt2432(
			reinterpret_cast<int32_t*>(data),
			buffer_.Get(),
			data_convert_);
		hr = render_client_->ReleaseBuffer(buffer_frames_, flags);
		return result;
	}
	// EOF data
	hr = render_client_->ReleaseBuffer(buffer_frames_, AUDCLNT_BUFFERFLAGS_SILENT);
	return false;
}

void ExclusiveWasapiDevice::SetAudioCallback(IAudioCallback* callback) noexcept {
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
	is_running_ = false;
}

void ExclusiveWasapiDevice::StartStream() {
	XAMP_LOG_D(log_, "StartStream!");

	if (!client_ || !render_client_) {
		throw HRException(AUDCLNT_E_NOT_INITIALIZED);
	}

	XAMP_ASSERT(sample_ready_);
	XAMP_ASSERT(thread_start_);
	XAMP_ASSERT(thread_exit_);
	XAMP_ASSERT(close_request_);

	const auto wait_timeout =
		std::chrono::milliseconds(static_cast<uint64_t>(Nano100ToMillis(aligned_period_)))
		+ std::chrono::milliseconds(20);
	XAMP_LOG_D(log_, "WASAPI wait timeout {}msec.", wait_timeout.count());
	::ResetEvent(close_request_.get());

	// Note: 必要! 某些音效卡會爆音!
	GetSample(true);

	render_task_ = GetWASAPIThreadPool().Spawn([this, wait_timeout](auto idx) noexcept {
		XAMP_LOG_D(log_, "Start exclusive mode stream task! thread: {}", GetCurrentThreadId());
		DWORD current_timeout = INFINITE;
		Stopwatch watch;
		Mmcss mmcss;

		is_running_ = true;

		mmcss.BoostPriority(mmcss_name_, thread_priority_);
		const std::array<HANDLE, 2> objects{
			sample_ready_.get(),
			close_request_.get()
		};		

		auto thread_exit = false;
		::SetEvent(thread_start_.get());

		while (!thread_exit) {
			watch.Reset();

			auto result = ::WaitForMultipleObjects(objects.size(), objects.data(), FALSE, current_timeout);

			const auto elapsed = watch.Elapsed<std::chrono::milliseconds>();
			if (elapsed > wait_timeout) {
				XAMP_LOG_D(log_, "WASAPI wait too slow! {}msec.", elapsed.count());
				//thread_exit = true;
				//continue;
			}

			switch (result) {
			case WAIT_OBJECT_0 + 0:
				if (!GetSample(false)) {
					thread_exit = true;
				}
				current_timeout = wait_timeout.count();
				break;
			case WAIT_OBJECT_0 + 1:
				thread_exit = true;
				XAMP_LOG_D(log_, "Stop event trigger!");
				break;
			case WAIT_TIMEOUT:
				XAMP_LOG_D(log_, "Wait event timeout! {}msec.", elapsed.count());
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
	// todo: GetVolume回傳level, CoreAudio, ASIO均無法讀取設備的dBFS
	//float volume_level = 0.0;
	//HrIfFailledThrow(endpoint_volume_->GetMasterVolumeLevel(&volume_level));
	auto volume_scalar = 0.0F;
	HrIfFailledThrow(endpoint_volume_->GetMasterVolumeLevelScalar(&volume_scalar));
	return static_cast<uint32_t>(volume_scalar * 100.0F);
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

	const auto volume_scalar = volume / 100.0F;
	HrIfFailledThrow(endpoint_volume_->SetMasterVolumeLevelScalar(volume_scalar, &GUID_NULL));

	XAMP_LOG_D(log_, "Current volume: {}.", GetVolume());
}

bool ExclusiveWasapiDevice::IsMuted() const {
	auto is_mute = FALSE;
	HrIfFailledThrow(endpoint_volume_->GetMute(&is_mute));
	return is_mute;
}

void ExclusiveWasapiDevice::SetMute(const bool mute) const {
	HrIfFailledThrow(endpoint_volume_->SetMute(mute, nullptr));
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
