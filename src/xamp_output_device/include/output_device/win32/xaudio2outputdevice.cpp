#include <output_device/win32/xaudio2outputdevice.h>

#include <base/executor.h>
#include <base/ithreadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/timer.h>
#include <base/scopeguard.h>
#include <base/ithreadpoolexecutor.h>

#include <output_device/iaudiocallback.h>
#include <output_device/win32/comexception.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN
namespace {
	/*
	* Set wave format.
	*/
	void SetWaveformatEx(WAVEFORMATEX& format, uint32_t sample_rate) noexcept {
		// Fixed float format.
		format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
		format.nChannels = 2;
		format.wBitsPerSample = 8 * sizeof(float);
		format.cbSize = 0;
		format.nSamplesPerSec = sample_rate;
		format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
	}
}

XAMP_DECLARE_LOG_NAME(XAudio2EngineContext);

class XAudio2OutputDevice::XAudio2EngineContext final : public IXAudio2EngineCallback {
public:
	explicit XAudio2EngineContext(const LoggerPtr& logger) {
		logger_ = logger;
	}

	void OnProcessingPassStart() override {
		XAMP_LOG_D(logger_, "OnProcessingPassStart");
	}

	void OnProcessingPassEnd() override {
		XAMP_LOG_D(logger_, "OnProcessingPassEnd");
	}

	void OnCriticalError(HRESULT Error) override {
		XAMP_LOG_D(logger_, "OnCriticalError");
	}
private:
	LoggerPtr logger_;
};

class XAudio2OutputDevice::XAudio2VoiceContext final : public IXAudio2VoiceCallback {
public:
	explicit XAudio2VoiceContext(const LoggerPtr &logger) {
		sample_ready_.reset(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
		logger_ = logger;
	}

	virtual ~XAudio2VoiceContext() = default;

	void OnVoiceProcessingPassStart(UINT32 BytesRequired) override {
		XAMP_LOG_D(logger_, "OnVoiceProcessingPassStart");
	}

	void OnVoiceProcessingPassEnd() override {
		XAMP_LOG_D(logger_, "OnVoiceProcessingPassEnd");
	}

	void OnStreamEnd() override {
		XAMP_LOG_D(logger_, "OnStreamEnd");
	}

	void OnBufferStart(void* pBufferContext) override {
		XAMP_LOG_D(logger_, "OnBufferStart");
	}

	void OnBufferEnd(void* pBufferContext) override {
		XAMP_LOG_D(logger_, "OnBufferEnd");
		::SetEvent(sample_ready_.get());
	}

	void OnLoopEnd(void* pBufferContext) override {
		XAMP_LOG_D(logger_, "OnLoopEnd");
	}

	void OnVoiceError(void* pBufferContext, HRESULT Error) override {
		XAMP_LOG_D(logger_, "OnVoiceError");
	}

	WinHandle sample_ready_;
private:	
	LoggerPtr logger_;
};

XAudio2OutputDevice::XAudio2OutputDevice(const std::shared_ptr<IThreadPoolExecutor>& thread_pool, const std::wstring& device_id)
	: is_running_(false)
	, buffer_frames_(0)
	, callback_(nullptr)
	, device_id_(device_id)
	, mastering_voice_(nullptr)
	, source_voice_(nullptr)
	, logger_(XampLoggerFactory.GetLogger(kXAudio2OutputDeviceLoggerName))
	, thread_pool_(thread_pool) {
#ifdef _DEBUG
	UINT32 flags = 0;
	HrIfFailThrow(::XAudio2Create(&xaudio2_, XAUDIO2_DEBUG_ENGINE, XAUDIO2_DEFAULT_PROCESSOR));

	XAUDIO2_DEBUG_CONFIGURATION debug_config;
	debug_config.TraceMask = XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_DETAIL | XAUDIO2_LOG_FUNC_CALLS | XAUDIO2_LOG_TIMING | XAUDIO2_LOG_LOCKS | XAUDIO2_LOG_MEMORY | XAUDIO2_LOG_STREAMING;
	debug_config.BreakMask = XAUDIO2_LOG_WARNINGS;
	debug_config.LogThreadID = TRUE;
	debug_config.LogFileline = TRUE;
	debug_config.LogFunctionName = TRUE;
	debug_config.LogTiming = TRUE;
	xaudio2_->SetDebugConfiguration(&debug_config, nullptr);
#else
	HrIfFailThrow(::XAudio2Create(&xaudio2_));
#endif
	auto context_logger = XampLoggerFactory.GetLogger(kXAudio2EngineContextLoggerName);
	engine_context_ = MakeAlign<XAudio2EngineContext>(context_logger);
	voice_context_ = MakeAlign<XAudio2VoiceContext>(context_logger);
	HrIfFailThrow(xaudio2_->RegisterForCallbacks(engine_context_.get()));
}

XAudio2OutputDevice::~XAudio2OutputDevice() {
	StopStream();
	CloseStream();
}

bool XAudio2OutputDevice::IsStreamOpen() const noexcept {
	return mastering_voice_ != nullptr;
}

void XAudio2OutputDevice::SetAudioCallback(IAudioCallback* callback) noexcept {
	XAMP_EXPECTS(callback != nullptr);
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
	}
	is_running_ = false;
}

void XAudio2OutputDevice::CloseStream() {
	if (source_voice_ != nullptr) {
		source_voice_->DestroyVoice();
		source_voice_ = nullptr;
	}

	if (xaudio2_ != nullptr) {
		xaudio2_->StopEngine();
	}

	if (mastering_voice_ != nullptr) {
		mastering_voice_->DestroyVoice();
		mastering_voice_ = nullptr;
	}

	thread_start_.close();
	thread_exit_.close();
	close_request_.close();
	render_task_ = Task<void>();
}

void XAudio2OutputDevice::OpenStream(AudioFormat const& output_format) {
	// NOTE: 設置太大或太小都會有問題.
	auto get_buffer_size = [](auto sample_rate) {
		return (sample_rate / 2) * 8;
		};

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

	if (!source_voice_) {
		WAVEFORMATEX waveformat{};
		SetWaveformatEx(waveformat, output_format_.GetSampleRate());

		HrIfFailThrow(xaudio2_->CreateSourceVoice(&source_voice_,
			&waveformat,
			XAUDIO2_VOICE_NOSRC |
			XAUDIO2_VOICE_NOPITCH,
			1,
			voice_context_.get()));
	}
}

bool XAudio2OutputDevice::IsMuted() const {
	return GetVolume() == 0;
}

void XAudio2OutputDevice::SetMute(bool mute) const {
	if (mute) {
		SetVolume(0);
	}
}

PackedFormat XAudio2OutputDevice::GetPackedFormat() const noexcept {
	return PackedFormat::INTERLEAVED;
}

uint32_t XAudio2OutputDevice::GetBufferSize() const noexcept {
	return buffer_frames_ * AudioFormat::kMaxChannel;
}

uint32_t XAudio2OutputDevice::GetVolume() const {
	if (!source_voice_) {
		return 0;
	}
	float volume = 0;
	source_voice_->GetVolume(&volume);
	uint32_t mapped_volume = static_cast<uint32_t>(volume * 100);
	return mapped_volume;
}

void XAudio2OutputDevice::SetVolume(uint32_t volume) const {
	if (!source_voice_) {
		return;
	}
	float mapped_volume = volume / 100.0f;
	HrIfFailThrow(source_voice_->SetVolume(mapped_volume));
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

	::ResetEvent(close_request_.get());

	if (!source_voice_) {
		WAVEFORMATEX waveformat{};
		SetWaveformatEx(waveformat, output_format_.GetSampleRate());

		HrIfFailThrow(xaudio2_->CreateSourceVoice(&source_voice_,
			&waveformat,
			XAUDIO2_VOICE_NOSRC |
			XAUDIO2_VOICE_NOPITCH,
			1,
			voice_context_.get()));
	}

	render_task_ = Executor::Spawn(thread_pool_.get(), [this](const auto& stop_token) {
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
				auto wait_result = ::WaitForMultipleObjects(static_cast<DWORD>(objects.size()),
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
			::WaitForMultipleObjects(static_cast<DWORD>(objects.size()),
				objects.data(),
				FALSE,
				INFINITE);
		}

		XAMP_LOG_D(logger_, "Render task done!");
	});

	if (::WaitForSingleObject(thread_start_.get(), kWaitThreadStartSecond) == WAIT_TIMEOUT) {
		throw_translated_com_error(HRESULT_FROM_WIN32(ERROR_TIMEOUT));
	}

	HrIfFailThrow(xaudio2_->StartEngine());
	HrIfFailThrow(source_voice_->Start(0, XAUDIO2_COMMIT_NOW));
}

HRESULT XAudio2OutputDevice::FillSamples(bool &end_of_stream) {
	XAUDIO2_BUFFER buffer{};
	size_t num_filled_frames = 0;

	float sample_time = 0;
	auto stream_time = stream_time_ + buffer_frames_;
	float stream_time_float = static_cast<float>(static_cast<double>(stream_time) / static_cast<double>(output_format_.GetSampleRate()));
	stream_time_ = stream_time;

	if (callback_->OnGetSamples(buffer_.Get(),
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

void XAudio2OutputDevice::ReportError(HRESULT hr) noexcept {
	if (FAILED(hr)) {
		callback_->OnError(com_to_system_error(hr));
		is_running_ = false;
	}
}

bool XAudio2OutputDevice::IsHardwareControlVolume() const {
	return false;
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END
