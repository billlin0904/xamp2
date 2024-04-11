#include <output_device/win32/xaudio2outputdevice.h>

#include <base/executor.h>
#include <base/ithreadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/timer.h>
#include <base/scopeguard.h>

#include <output_device/iaudiocallback.h>
#include <output_device/win32/comexception.h>

#include <output_device/win32/mmcss.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN
namespace {
	void SetWaveformatEx(WAVEFORMATEX& format, uint32_t samplerate) noexcept {
		format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
		format.nChannels = 2;
		format.wBitsPerSample = 8 * sizeof(float);
		format.cbSize = 0;
		format.nSamplesPerSec = samplerate;
		format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
	}
}

class XAudio2OutputDevice::XAudio2EngineContext final : public IXAudio2EngineCallback {
public:
	void OnProcessingPassStart() override {
		//XAMP_LOG_DEBUG("OnProcessingPassStart");
	}

	void OnProcessingPassEnd() override {
		//XAMP_LOG_DEBUG("OnProcessingPassEnd");
	}

	void OnCriticalError(HRESULT Error) override {
		//XAMP_LOG_DEBUG("OnCriticalError");
	}
};

class XAudio2OutputDevice::XAudio2VoiceContext final : public IXAudio2VoiceCallback {
public:
	XAudio2VoiceContext() {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
	}

	virtual ~XAudio2VoiceContext() = default;

	void OnVoiceProcessingPassStart(UINT32 BytesRequired) override {
		//XAMP_LOG_DEBUG("OnVoiceProcessingPassStart");
	}

	void OnVoiceProcessingPassEnd() override {
		//XAMP_LOG_DEBUG("OnVoiceProcessingPassEnd");
	}

	void OnStreamEnd() override {
		//XAMP_LOG_DEBUG("OnStreamEnd");
	}

	void OnBufferStart(void* pBufferContext) override {
		//XAMP_LOG_DEBUG("OnBufferStart");
	}

	void OnBufferEnd(void* pBufferContext) override {
		::SetEvent(sample_ready_.get());
	}

	void OnLoopEnd(void* pBufferContext) override {
		//XAMP_LOG_DEBUG("OnLoopEnd");
	}

	void OnVoiceError(void* pBufferContext, HRESULT Error) override {
		//XAMP_LOG_DEBUG("OnVoiceError");
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
	, device_id_(device_id)
	, xaudio2_(xaudio2)
	, mastering_voice_(nullptr)
	, logger_(XampLoggerFactory.GetLogger(kXAudio2OutputDeviceLoggerName)) {
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

	if (source_voice_ != nullptr) {
		source_voice_->Stop();
		source_voice_->FlushSourceBuffers();
		source_voice_->DestroyVoice();
		source_voice_ = nullptr;
	}

	if (mastering_voice_ != nullptr) {
		mastering_voice_->DestroyVoice();
		mastering_voice_ = nullptr;
	}
	is_running_ = false;
}

void XAudio2OutputDevice::CloseStream() {
	if (xaudio2_ != nullptr) {
		xaudio2_->StopEngine();
	}

	engine_context_.reset();
	voice_context_.reset();
	thread_start_.close();
	thread_exit_.close();
	close_request_.close();
	render_task_ = Task<void>();
}

void XAudio2OutputDevice::OpenStream(AudioFormat const& output_format) {
	auto get_buffer_size = [](auto sample_rate) {
		return (sample_rate / 2) * 8;
		};

	engine_context_ = MakeAlign<XAudio2EngineContext>();
	voice_context_ = MakeAlign<XAudio2VoiceContext>();

	HrIfFailThrow(xaudio2_->RegisterForCallbacks(engine_context_.get()));

	HrIfFailThrow(xaudio2_->CreateMasteringVoice(&mastering_voice_,
		output_format.GetChannels(),
		output_format.GetSampleRate(),
		0,
		device_id_.c_str(),
		nullptr));

	// Create thread start event handle.
	if (!thread_start_) {
		thread_start_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
	}

	// Create thread exit event handle.
	if (!thread_exit_) {
		thread_exit_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
	}

	// Create close request event handle.
	if (!close_request_) {
		close_request_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
	}

	output_format_ = output_format;
	buffer_frames_ = get_buffer_size(output_format.GetSampleRate()) / output_format.GetChannels() / output_format.GetBytesPerSample();
	buffer_.resize(get_buffer_size(output_format.GetSampleRate()) / output_format.GetChannels());
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
	XAMP_EXPECTS(voice_context_->sample_ready_);
	XAMP_EXPECTS(thread_start_);
	XAMP_EXPECTS(thread_exit_);
	XAMP_EXPECTS(close_request_);

	WAVEFORMATEX waveformat{};
	SetWaveformatEx(waveformat, output_format_.GetSampleRate());

	HrIfFailThrow(xaudio2_->CreateSourceVoice(&source_voice_,
		&waveformat,
		XAUDIO2_VOICE_NOSRC |
		XAUDIO2_VOICE_NOPITCH,
		1,
		voice_context_.get()));

	XAUDIO2_VOICE_DETAILS details{};
	source_voice_->GetVoiceDetails(&details);

	::ResetEvent(close_request_.get());

	bool dummy = false;
	HrIfFailThrow(FillSamples(dummy));

	render_task_ = Executor::Spawn(GetOutputDeviceThreadPool(), [this](const auto& stop_token) {
		is_running_ = true;

		const std::array<HANDLE, 2> objects{
			// WAIT_OBJECT_0
			voice_context_->sample_ready_.get(),
			// WAIT_OBJECT_0 + 1
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
			while (true) {
				XAUDIO2_VOICE_STATE state{};
				source_voice_->GetState(&state);
				if (!state.BuffersQueued) {
					break;
				}
				auto wait_result = ::WaitForMultipleObjects(objects.size(),
					objects.data(),
					FALSE, 
					INFINITE);
				if (wait_result == WAIT_OBJECT_0 + 1) {
					thread_exit = true;
					break;
				}
			}

			if (thread_exit || stop_token.stop_requested()) {
				continue;
			}

			auto hr = FillSamples(thread_exit);
			if (!thread_exit) {
				ReportError(hr);
				thread_exit = FAILED(hr);
			}
		}

		while (true) {
			XAUDIO2_VOICE_STATE state{};
			source_voice_->GetState(&state);
			if (!state.BuffersQueued) {
				break;
			}
			::WaitForMultipleObjects(objects.size(),
				objects.data(),
				FALSE,
				INFINITE);
		}
	});

	if (::WaitForSingleObject(thread_start_.get(), kWaitThreadStartSecond) == WAIT_TIMEOUT) {
		throw ComException(HRESULT_FROM_WIN32(ERROR_TIMEOUT));
	}

	HrIfFailThrow(xaudio2_->StartEngine());
	HrIfFailThrow(source_voice_->Start(0, XAUDIO2_COMMIT_NOW));
}

HRESULT XAudio2OutputDevice::FillSamples(bool &end_of_stream) {
	XAUDIO2_BUFFER buffer{};
	size_t num_filled_frames = 0;

	float sample_time = 0;
	auto stream_time = stream_time_ + buffer_frames_;
	float stream_time_float = static_cast<double>(stream_time) / static_cast<double>(output_format_.GetSampleRate());
	stream_time_ = stream_time;

	XAMP_LIKELY(callback_->OnGetSamples(buffer_.Get(),
		buffer_frames_,
		num_filled_frames,
		stream_time_float,
		sample_time) == DataCallbackResult::CONTINUE) {
		buffer.AudioBytes = num_filled_frames * output_format_.GetChannels() * output_format_.GetBytesPerSample();
		buffer.pAudioData = reinterpret_cast<const BYTE*>(buffer_.Get());
		buffer.pContext = this;
		end_of_stream = false;
		return source_voice_->SubmitSourceBuffer(&buffer);
	}
	else {
		buffer.Flags = XAUDIO2_END_OF_STREAM;
		end_of_stream = true;
		return source_voice_->SubmitSourceBuffer(&buffer);
	}
}

bool XAudio2OutputDevice::IsStreamRunning() const noexcept {
	return is_running_;
}

void XAudio2OutputDevice::AbortStream() noexcept {
	is_running_ = false;
}

void XAudio2OutputDevice::SetVolumeLevelScalar(float level) {
}

void XAudio2OutputDevice::ReportError(HRESULT hr) noexcept {
	if (FAILED(hr)) {
		const ComException exception(hr);
		callback_->OnError(exception);
		is_running_ = false;
	}
}

bool XAudio2OutputDevice::IsHardwareControlVolume() const {
	return false;
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END
