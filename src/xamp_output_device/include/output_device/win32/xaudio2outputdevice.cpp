#include <output_device/win32/xaudio2outputdevice.h>

#include <base/executor.h>
#include <base/ithreadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/timer.h>
#include <base/scopeguard.h>

#include <output_device/iaudiocallback.h>
#include <output_device/win32/comexception.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN
	namespace {
	/*
	* Set WAVEFORMATEX from AudioFormat and valid bits samples
	*
	* @param input_fromat: input format
	* @param audio_format: audio format
	*/
	void SetWaveformatEx(WAVEFORMATEX* input_fromat, const AudioFormat& audio_format, const int32_t valid_bits_samples) noexcept {
		XAMP_EXPECTS(input_fromat != nullptr);
		XAMP_EXPECTS(audio_format.GetChannels() == AudioFormat::kMaxChannel);
		XAMP_EXPECTS(valid_bits_samples > 0);

		// Check if this is correct	
		XAMP_EXPECTS(input_fromat->cbSize == sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX));
		auto& format = *reinterpret_cast<WAVEFORMATEXTENSIBLE*>(input_fromat);

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
}

class XAudio2OutputDevice::XAudio2VoiceContext final : public IXAudio2VoiceCallback {
public:
	XAudio2VoiceContext() {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
	}

	virtual ~XAudio2VoiceContext() = default;

	void OnVoiceProcessingPassStart(UINT32 BytesRequired) override {

	}

	void OnVoiceProcessingPassEnd() override {

	}

	void OnStreamEnd() override {
		::SetEvent(sample_ready_.get());
	}

	void OnBufferStart(void* pBufferContext) override {

	}

	void OnBufferEnd(void* pBufferContext) override {

	}

	void OnLoopEnd(void* pBufferContext) override {

	}
	void OnVoiceError(void* pBufferContext, HRESULT Error) override {

	}
	WinHandle sample_ready_;
};

XAudio2OutputDevice::XAudio2OutputDevice(const CComPtr<IXAudio2>& xaudio2, const std::wstring& device_id)
	: is_running_(false)
	, raw_mode_(false)
	, is_muted_(false)
	, is_stopped_(true)
	, volume_(0)
	, buffer_frames_(0)
	, callback_(nullptr)
	, wait_time_(0)
	, logger_(XampLoggerFactory.GetLogger(kXAudio2OutputDeviceLoggerName))
	, device_id_(device_id)
	, xaudio2_(xaudio2)
	, mastering_voice_(nullptr) {
	context_ = MakeAlign<XAudio2VoiceContext>();
}

XAudio2OutputDevice::~XAudio2OutputDevice() {
	CloseStream();
	if (mastering_voice_ != nullptr) {
		mastering_voice_->DestroyVoice();
	}
	xaudio2_.Release();
}

bool XAudio2OutputDevice::IsStreamOpen() const noexcept {
	return true;
}

void XAudio2OutputDevice::SetAudioCallback(IAudioCallback* callback) noexcept {
	callback_ = callback;
}

void XAudio2OutputDevice::StopStream(bool wait_for_stop_stream) {
	if (!is_running_) {
		return;
	}

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

void XAudio2OutputDevice::CloseStream() {
	if (source_voice_ != nullptr) {
		source_voice_->Stop();
		source_voice_->DestroyVoice();
	}
	render_task_ = Task<void>();
}

void XAudio2OutputDevice::OpenStream(AudioFormat const& output_format) {
	auto hr = xaudio2_->CreateMasteringVoice(&mastering_voice_,
		XAUDIO2_DEFAULT_CHANNELS,
		XAUDIO2_DEFAULT_SAMPLERATE,
		0,
		device_id_.c_str(),
		nullptr);
	HrIfFailThrow(hr);
	WAVEFORMATEXTENSIBLE waveformatex{};
	waveformatex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	SetWaveformatEx(reinterpret_cast<WAVEFORMATEX*>(&waveformatex), output_format, 32);
	hr = xaudio2_->CreateSourceVoice(&source_voice_,
		reinterpret_cast<WAVEFORMATEX*>(&waveformatex),		
		0, 
		1,
		context_.get());
	HrIfFailThrow(hr);
}

bool XAudio2OutputDevice::IsMuted() const {
	return is_muted_;
}

uint32_t XAudio2OutputDevice::GetVolume() const {
	return volume_;
}

void XAudio2OutputDevice::SetMute(bool mute) const {
	is_muted_ = mute;
}

PackedFormat XAudio2OutputDevice::GetPackedFormat() const noexcept {
	return PackedFormat::INTERLEAVED;
}

uint32_t XAudio2OutputDevice::GetBufferSize() const noexcept {
	return buffer_frames_ * AudioFormat::kMaxChannel;
}

void XAudio2OutputDevice::SetVolume(uint32_t volume) const {
	volume_ = std::clamp(volume, static_cast<uint32_t>(0), static_cast<uint32_t>(100));
}

void XAudio2OutputDevice::SetStreamTime(double stream_time) noexcept {
	stream_time_ = static_cast<int64_t>(stream_time
		* static_cast<double>(output_format_.GetSampleRate()));
}

double XAudio2OutputDevice::GetStreamTime() const noexcept {
	return stream_time_ / static_cast<double>(output_format_.GetSampleRate());
}

void XAudio2OutputDevice::StartStream() {
	::ResetEvent(close_request_.get());

	render_task_ = Executor::Spawn(GetWasapiThreadPool(), [this](const auto& stop_token) {
		const std::array<HANDLE, 2> objects{
			context_->sample_ready_.get(),
			close_request_.get()
		};

		auto thread_exit = false;
		::SetEvent(thread_start_.get());

		XAMP_ON_SCOPE_EXIT(
			// Signal thread exit.
			::SetEvent(thread_exit_.get());
			XAMP_LOG_D(logger_, "End XAudio2 stream task!");
		);

		while (!thread_exit && !stop_token.stop_requested()) {
			while (!thread_exit && !stop_token.stop_requested()) {
				XAUDIO2_VOICE_STATE state;
				source_voice_->GetState(&state);
				if (state.BuffersQueued < 1)
					break;
				auto wait_result = ::WaitForMultipleObjects(objects.size(), objects.data(), FALSE, INFINITE);
				if (wait_result == WAIT_OBJECT_0 + 1) {
					thread_exit = true;
					return;
				}
			}

			XAUDIO2_BUFFER buf{};
			size_t num_filled_frames = 0;
			float stream_time_float = 0;
			float sample_time = 0;

			XAMP_LIKELY(callback_->OnGetSamples(buffer_.Get(), buffer_frames_,
				num_filled_frames,
				stream_time_float,
				sample_time) == DataCallbackResult::CONTINUE) {
				buf.AudioBytes = num_filled_frames;
				buf.pAudioData = reinterpret_cast<const BYTE*>(buffer_.Get());
				source_voice_->SubmitSourceBuffer(&buf);
			} else {
				buf.Flags = XAUDIO2_END_OF_STREAM;
				source_voice_->SubmitSourceBuffer(&buf);
			}

			while (!thread_exit && !stop_token.stop_requested()) {
				XAUDIO2_VOICE_STATE state;
				source_voice_->GetState(&state);
				if (state.BuffersQueued < 1)
					break;
				auto wait_result = ::WaitForMultipleObjects(objects.size(), objects.data(), FALSE, INFINITE);
				if (wait_result == WAIT_OBJECT_0 + 1) {
					thread_exit = true;
					break;
				}
			}
		}

	});

	// Wait thread start.
	constexpr auto kWaitThreadStartSecond = 60 * 1000; // 60sec
	if (::WaitForSingleObject(thread_start_.get(), kWaitThreadStartSecond) == WAIT_TIMEOUT) {
		throw ComException(HRESULT_FROM_WIN32(ERROR_TIMEOUT));
	}
	HrIfFailThrow(source_voice_->Start());
}

bool XAudio2OutputDevice::IsStreamRunning() const noexcept {
	return is_running_;
}

void XAudio2OutputDevice::AbortStream() noexcept {
	is_running_ = false;
}

void XAudio2OutputDevice::SetVolumeLevelScalar(float level) {
}

bool XAudio2OutputDevice::IsHardwareControlVolume() const {
	return false;
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END
