//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <output_device/posix/alsaoutputdevice.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <thread>

#include <alsa/asoundlib.h>

#include <base/exception.h>
#include <base/logger.h>
#include <base/volume.h>
#include <base/unique_handle.h>

#include <output_device/iaudiocallback.h>
#include <output_device/posix/thread_priority.h>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

namespace {
constexpr uint32_t kDefaultPeriodMilliseconds = 50;
constexpr uint32_t kMicrosecondsPerMillisecond = 1000;
constexpr uint32_t kBufferPeriodCount = 4;
constexpr int kPollTimeoutMilliseconds = 100;
constexpr auto kAlsaDefaultDeviceId = "default";
constexpr auto kSuspendResumeRetryDelay = std::chrono::milliseconds(100);

snd_pcm_uframes_t GetPeriodFrames(uint32_t sample_rate) {
	const auto frames = static_cast<snd_pcm_uframes_t>(sample_rate) * kDefaultPeriodMilliseconds / 1000;
	return std::max<snd_pcm_uframes_t>(frames, 1);
}

double FramesToMilliseconds(snd_pcm_uframes_t frames, uint32_t sample_rate) {
	if (sample_rate == 0) {
		return 0.0;
	}
	return static_cast<double>(frames) * 1000.0 / static_cast<double>(sample_rate);
}

snd_pcm_format_t ToAlsaSampleFormat(const AudioFormat& format) {
	if (format.GetFormat() != DataFormat::FORMAT_PCM) {
		throw DeviceUnSupportedFormatException(format);
	}

	if (format.GetPackedFormat() != PackedFormat::INTERLEAVED) {
		throw DeviceUnSupportedFormatException(format);
	}

	switch (format.GetByteFormat()) {
	case ByteFormat::FLOAT32:
		return SND_PCM_FORMAT_FLOAT_LE;
	default:
		throw DeviceUnSupportedFormatException(format);
	}
}

void ThrowAlsaError(std::string_view operation, int error) {
	Throw<PlatformException>("ALSA {} failed: {}", operation, ::snd_strerror(error));
}
}

void AlsaOutputDevice::SndPcmHandleTraits::Close(snd_pcm_t* value) {
	(void)::snd_pcm_close(value);
}

AlsaOutputDevice::AlsaOutputDevice(const std::shared_ptr<IThreadPoolExecutor>& thread_pool,
	std::string device_id)
	: device_id_(std::move(device_id))
	, thread_pool_(thread_pool)
	, logger_(XampLoggerFactory.GetLogger(XAMP_LOG_NAME(AlsaOutputDevice))) {
	if (device_id_.empty()) {
		device_id_ = kAlsaDefaultDeviceId;
	}
	logger_->SetLevel(LogLevel::LOG_LEVEL_DEBUG);
}

AlsaOutputDevice::~AlsaOutputDevice() {
	try {
		CloseStream();
	}
	catch (...) {
	}
}

void AlsaOutputDevice::OpenStream(const AudioFormat& output_format) {
	CloseStream();

	XAMP_LOG_D(logger_, "AlsaOutputDevice open stream: {}.", output_format.ToString());

	const auto sample_format = ToAlsaSampleFormat(output_format);
	snd_pcm_t* pcm = nullptr;
	auto error = ::snd_pcm_open(&pcm,
		device_id_.empty() ? kAlsaDefaultDeviceId : device_id_.c_str(),
		SND_PCM_STREAM_PLAYBACK,
		SND_PCM_NONBLOCK);
	if (error < 0) {
		ThrowAlsaError("open", error);
	}
	pcm_.reset(pcm);

	try {
		snd_pcm_hw_params_t* hw_params = nullptr;
		snd_pcm_hw_params_alloca(&hw_params);

		if ((error = ::snd_pcm_hw_params_any(pcm_.get(), hw_params)) < 0) {
			ThrowAlsaError("hw params any", error);
		}
		if ((error = ::snd_pcm_hw_params_set_access(pcm_.get(), hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			ThrowAlsaError("set access", error);
		}
		if ((error = ::snd_pcm_hw_params_set_format(pcm_.get(), hw_params, sample_format)) < 0) {
			ThrowAlsaError("set format", error);
		}
		if ((error = ::snd_pcm_hw_params_set_channels(pcm_.get(), hw_params, output_format.GetChannels())) < 0) {
			ThrowAlsaError("set channels", error);
		}

		error = ::snd_pcm_hw_params_set_rate_resample(pcm_.get(), hw_params, 0);
		if (error < 0) {
			XAMP_LOG_D(logger_, "ALSA disable resampling unavailable: {}.", ::snd_strerror(error));
		}

		auto sample_rate = output_format.GetSampleRate();
		if ((error = ::snd_pcm_hw_params_set_rate_near(pcm_.get(), hw_params, &sample_rate, nullptr)) < 0) {
			ThrowAlsaError("set rate", error);
		}
		if (sample_rate != output_format.GetSampleRate()) {
			Throw<PlatformException>("ALSA sample rate mismatch: requested:{} actual:{}.",
				output_format.GetSampleRate(),
				sample_rate);
		}

		unsigned int buffer_time = kDefaultPeriodMilliseconds * kBufferPeriodCount * kMicrosecondsPerMillisecond;
		int buffer_time_dir = 0;
		error = ::snd_pcm_hw_params_set_buffer_time_near(pcm_.get(), hw_params, &buffer_time, &buffer_time_dir);
		if (error < 0) {
			XAMP_LOG_D(logger_, "ALSA set buffer time unavailable: {}.", ::snd_strerror(error));
		}

		unsigned int period_time = kDefaultPeriodMilliseconds * kMicrosecondsPerMillisecond;
		int period_time_dir = 0;
		error = ::snd_pcm_hw_params_set_period_time_near(pcm_.get(), hw_params, &period_time, &period_time_dir);
		if (error < 0) {
			XAMP_LOG_D(logger_, "ALSA set period time unavailable: {}.", ::snd_strerror(error));
		}

		const auto requested_period_frames = GetPeriodFrames(sample_rate);
		snd_pcm_uframes_t period_frames = requested_period_frames;
		if (error < 0) {
			(void)::snd_pcm_hw_params_set_period_size_near(pcm_.get(), hw_params, &period_frames, nullptr);
		}

		snd_pcm_uframes_t buffer_frames = period_frames * kBufferPeriodCount;
		if (buffer_time == 0 || period_time == 0) {
			(void)::snd_pcm_hw_params_set_buffer_size_near(pcm_.get(), hw_params, &buffer_frames);
		}

		if ((error = ::snd_pcm_hw_params(pcm_.get(), hw_params)) < 0) {
			ThrowAlsaError("apply hw params", error);
		}

		(void)::snd_pcm_hw_params_get_period_size(hw_params, &period_frames, nullptr);
		(void)::snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_frames);
		const auto safe_period_frames = std::max<snd_pcm_uframes_t>(period_frames, 1);
		const auto safe_buffer_frames = std::max(buffer_frames, safe_period_frames);
		buffer_frames_ = static_cast<uint32_t>((std::max)(requested_period_frames, safe_period_frames));
		device_buffer_frames_ = static_cast<uint32_t>(safe_buffer_frames);

		snd_pcm_sw_params_t* sw_params = nullptr;
		snd_pcm_sw_params_alloca(&sw_params);
		if ((error = ::snd_pcm_sw_params_current(pcm_.get(), sw_params)) < 0) {
			ThrowAlsaError("sw params current", error);
		}
		if ((error = ::snd_pcm_sw_params_set_avail_min(pcm_.get(), sw_params, safe_period_frames)) < 0) {
			ThrowAlsaError("set avail min", error);
		}

		const auto start_threshold = (safe_buffer_frames / safe_period_frames) * safe_period_frames;
		if ((error = ::snd_pcm_sw_params_set_start_threshold(pcm_.get(), sw_params, start_threshold)) < 0) {
			ThrowAlsaError("set start threshold", error);
		}
		if ((error = ::snd_pcm_sw_params(pcm_.get(), sw_params)) < 0) {
			ThrowAlsaError("apply sw params", error);
		}

		if ((error = ::snd_pcm_prepare(pcm_.get())) < 0) {
			ThrowAlsaError("prepare", error);
		}

		const auto poll_count = ::snd_pcm_poll_descriptors_count(pcm_.get());
		if (poll_count <= 0) {
			ThrowAlsaError("poll descriptors count", poll_count);
		}

		output_format_ = output_format;
		render_buffer_ = MakeBuffer<float>(static_cast<size_t>(buffer_frames_) * output_format_.GetChannels());
		render_buffer_.Fill(0.0f);
		poll_descriptors_.resize(static_cast<size_t>(poll_count));
		stop_requested_ = true;
		is_running_ = false;

		XAMP_LOG_D(logger_,
			"ALSA stream ready device:{} callback_frames:{} alsa_period_frames:{}({:.2f}ms) buffer_frames:{}({:.2f}ms) start_threshold:{}.",
			device_id_,
			buffer_frames_,
			static_cast<uint32_t>(safe_period_frames),
			FramesToMilliseconds(safe_period_frames, sample_rate),
			device_buffer_frames_,
			FramesToMilliseconds(safe_buffer_frames, sample_rate),
			static_cast<uint32_t>(start_threshold));
	}
	catch (...) {
		CloseStream();
		throw;
	}
}

void AlsaOutputDevice::SetAudioCallback(IAudioCallback* callback) {
	callback_ = callback;
}

bool AlsaOutputDevice::IsStreamOpen() const {
	return pcm_.is_valid();
}

bool AlsaOutputDevice::IsStreamRunning() const {
	return is_running_;
}

void AlsaOutputDevice::StopRenderThread(bool wait_for_stop_stream) {
	stop_requested_ = true;
	is_running_ = false;

	if (pcm_) {
		(void)::snd_pcm_drop(pcm_.get());
	}

	if (!render_future_.valid()) {
		return;
	}

	if (wait_for_stop_stream) {
		render_future_.wait();
	}
	else {
		render_future_.wait();
	}
}

void AlsaOutputDevice::StopStream(bool wait_for_stop_stream) {
	if (!IsStreamOpen()) {
		return;
	}
	StopRenderThread(wait_for_stop_stream);
}

void AlsaOutputDevice::CloseStream() {
	StopRenderThread(true);
	render_buffer_.reset();
	poll_descriptors_.clear();

	pcm_.reset();
}

void AlsaOutputDevice::StartStream() {
	if (!IsStreamOpen() || is_running_) {
		return;
	}

	if (callback_ == nullptr) {
		Throw<PlatformException>("ALSA callback is not set.");
	}

	if (thread_pool_ == nullptr) {
		Throw<PlatformException>("ALSA thread pool is not set.");
	}

	if (render_future_.valid()) {
		render_future_.wait();
	}

	stop_requested_ = false;
	is_running_ = true;
	realtime_priority_configured_ = false;

	const auto error = ::snd_pcm_prepare(pcm_.get());
	if (error < 0) {
		is_running_ = false;
		stop_requested_ = true;
		ThrowAlsaError("prepare", error);
	}

	render_future_ = thread_pool_->Spawn([this](const std::stop_token& stop_token) {
		XAMP_LOG_D(logger_, "Render loop start");
		RenderLoop(stop_token);
		XAMP_LOG_D(logger_, "Render loop stop");
	}, ExecuteFlags::EXECUTE_LONG_RUNNING);
}

void AlsaOutputDevice::SetStreamTime(double stream_time) {
	const auto frame = static_cast<int64_t>(stream_time * output_format_.GetSampleRate());
	stream_frame_ = frame;
	stream_time_offset_frame_ = frame;
}

double AlsaOutputDevice::GetStreamTime() const {
	if (output_format_.GetSampleRate() == 0) {
		return 0;
	}
	return static_cast<double>(stream_frame_.load()) / output_format_.GetSampleRate();
}

uint32_t AlsaOutputDevice::GetVolume() const {
	return volume_;
}

void AlsaOutputDevice::SetVolume(uint32_t volume) const {
	volume_ = std::clamp(volume, 0U, 100U);
}

void AlsaOutputDevice::SetMute(bool mute) const {
	is_muted_ = mute;
}

bool AlsaOutputDevice::IsMuted() const {
	return is_muted_;
}

bool AlsaOutputDevice::IsHardwareControlVolume() const {
	return false;
}

PackedFormat AlsaOutputDevice::GetPackedFormat() const {
	return PackedFormat::INTERLEAVED;
}

uint32_t AlsaOutputDevice::GetBufferSize() const {
	return buffer_frames_ * AudioFormat::kMaxChannel;
}

void AlsaOutputDevice::AbortStream() {
	if (!IsStreamOpen()) {
		return;
	}
	StopRenderThread(true);
}

void AlsaOutputDevice::ApplySoftwareVolume(float* samples, size_t sample_count) const {
	if (output_format_.GetByteFormat() != ByteFormat::FLOAT32) {
		return;
	}

	if (is_muted_) {
		std::fill_n(samples, sample_count, 0.0f);
		return;
	}

	const auto volume = volume_.load();
	if (volume >= 100) {
		return;
	}

	const auto scale = VolumeLevelToGain(static_cast<int32_t>(volume));
	for (size_t i = 0; i < sample_count; ++i) {
		samples[i] *= scale;
	}
}

void AlsaOutputDevice::ConfigureRealtimeThreadPriority() {
	bool expected = false;
	if (!realtime_priority_configured_.compare_exchange_strong(expected, true)) {
		return;
	}

	SetRealtimeThreadPriority("ALSA render thread");
}

void AlsaOutputDevice::HandleRecoverableError(int error) {
	const auto state = ::snd_pcm_state(pcm_.get());
	if (error == -EPIPE || state == SND_PCM_STATE_XRUN) {
		XAMP_LOG_D(logger_, "ALSA xrun recovery state:{} error:{}.", ::snd_pcm_state_name(state), error);
		const auto prepare_result = ::snd_pcm_prepare(pcm_.get());
		if (prepare_result < 0) {
			ThrowAlsaError("recover prepare", prepare_result);
		}
		return;
	}

	if (error == -ESTRPIPE || state == SND_PCM_STATE_SUSPENDED) {
		XAMP_LOG_D(logger_, "ALSA suspend recovery state:{} error:{}.", ::snd_pcm_state_name(state), error);
		int resume_result = 0;
		do {
			resume_result = ::snd_pcm_resume(pcm_.get());
			if (resume_result == -EAGAIN) {
				std::this_thread::sleep_for(kSuspendResumeRetryDelay);
			}
		} while (resume_result == -EAGAIN && !stop_requested_);

		if (resume_result < 0) {
			const auto prepare_result = ::snd_pcm_prepare(pcm_.get());
			if (prepare_result < 0) {
				ThrowAlsaError("recover prepare", prepare_result);
			}
		}
		return;
	}

	const auto recover_result = ::snd_pcm_recover(pcm_.get(), error, 1);
	if (recover_result < 0) {
		ThrowAlsaError("recover", recover_result);
	}
}

bool AlsaOutputDevice::IsRecoverableState() const {
	const auto state = ::snd_pcm_state(pcm_.get());
	return state == SND_PCM_STATE_XRUN || state == SND_PCM_STATE_SUSPENDED;
}

bool AlsaOutputDevice::WaitUntilWritable(const std::stop_token& stop_token) {
	if (!pcm_ || poll_descriptors_.empty()) {
		return false;
	}

	while (!stop_requested_ && !stop_token.stop_requested()) {
		const auto descriptor_count = ::snd_pcm_poll_descriptors(pcm_.get(),
			poll_descriptors_.data(),
			static_cast<unsigned int>(poll_descriptors_.size()));
		if (descriptor_count < 0) {
			ThrowAlsaError("poll descriptors", descriptor_count);
		}

		const auto poll_result = ::poll(poll_descriptors_.data(),
			static_cast<nfds_t>(descriptor_count),
			kPollTimeoutMilliseconds);
		if (poll_result == 0) {
			continue;
		}
		if (poll_result < 0) {
			if (errno == EINTR) {
				continue;
			}
			Throw<PlatformException>("ALSA poll failed: {}", std::strerror(errno));
		}

		unsigned short revents = 0;
		const auto revents_result = ::snd_pcm_poll_descriptors_revents(pcm_.get(),
			poll_descriptors_.data(),
			static_cast<unsigned int>(descriptor_count),
			&revents);
		if (revents_result < 0) {
			ThrowAlsaError("poll descriptors revents", revents_result);
		}

		if ((revents & POLLERR) != 0) {
			if (IsRecoverableState()) {
				const auto state = ::snd_pcm_state(pcm_.get());
				HandleRecoverableError(state == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE);
				continue;
			}
			Throw<PlatformException>("ALSA poll reported POLLERR state:{}.", ::snd_pcm_state_name(::snd_pcm_state(pcm_.get())));
		}

		if ((revents & (POLLHUP | POLLNVAL)) != 0) {
			const auto available = ::snd_pcm_avail_update(pcm_.get());
			if (available < 0) {
				HandleRecoverableError(static_cast<int>(available));
				continue;
			}
			if (available > 0) {
				return true;
			}
		}

		if ((revents & POLLOUT) != 0) {
			return true;
		}
	}

	return false;
}

double AlsaOutputDevice::GetCallbackStreamTime(int64_t next_frame) const {
	const auto sample_rate = output_format_.GetSampleRate();
	if (sample_rate == 0) {
		return 0;
	}
	return static_cast<double>(next_frame) / sample_rate;
}

void AlsaOutputDevice::RenderLoop(const std::stop_token& stop_token) {
	ConfigureRealtimeThreadPriority();

	try {
		bool initial_fill = true;
		while (!stop_requested_ && !stop_token.stop_requested()) {
			if (!initial_fill && !WaitUntilWritable(stop_token)) {
				break;
			}

			const auto frames_to_write = static_cast<snd_pcm_uframes_t>(buffer_frames_);
			const auto channels = output_format_.GetChannels();
			auto* const samples = render_buffer_.data();
			render_buffer_.Fill(0.0f);

			size_t num_filled_frames = 0;
			const auto current_frame = stream_frame_.load();
			const auto next_frame = current_frame + static_cast<int64_t>(frames_to_write);
			const auto stream_time = GetCallbackStreamTime(next_frame);
			const auto result = callback_->OnGetSamples(samples,
				frames_to_write,
				num_filled_frames,
				stream_time,
				GetStreamTime());

			//XAMP_LOG_DEBUG("{} : callback result: {}, requested frames: {}, filled frames: {}, stream time: {:.2f}ms.",
			//	__func__,
			//	result == DataCallbackResult::CONTINUE ? "CONTINUE" : "STOP",
			//	static_cast<uint32_t>(frames_to_write),
			//	static_cast<uint32_t>(num_filled_frames),
			//	stream_time * 1000.0);

			num_filled_frames = std::min<size_t>(num_filled_frames, frames_to_write);
			if (result != DataCallbackResult::CONTINUE && num_filled_frames == 0) {
				break;
			}

			if (num_filled_frames == 0 && result == DataCallbackResult::CONTINUE) {
				num_filled_frames = frames_to_write;
			}
			if (num_filled_frames != frames_to_write) {
				XAMP_LOG_D(logger_,
					"ALSA underrun: requested:{} filled:{}. Fill remaining frames with silence.",
					static_cast<uint32_t>(frames_to_write),
					num_filled_frames);
			}

			const auto frames_ready = result == DataCallbackResult::CONTINUE
				? static_cast<size_t>(frames_to_write)
				: num_filled_frames;
			if (frames_ready == 0) {
				break;
			}

			ApplySoftwareVolume(samples, frames_ready * channels);

			auto frames_remaining = static_cast<snd_pcm_sframes_t>(frames_ready);
			auto* write_ptr = samples;
			while (frames_remaining > 0 && !stop_requested_ && !stop_token.stop_requested()) {
				const auto written = ::snd_pcm_writei(pcm_.get(), write_ptr, frames_remaining);
				if (written == -EAGAIN) {
					XAMP_LOG_D(logger_, "ALSA write would block, waiting for writable...");
					if (!WaitUntilWritable(stop_token)) {
						break;
					}
					continue;
				}
				//XAMP_LOG_D(logger_, "ALSA write result: {} frames.", written);
				if (written < 0) {
					HandleRecoverableError(static_cast<int>(written));
					initial_fill = true;
					break;
				}
				if (written == 0) {
					if (!WaitUntilWritable(stop_token)) {
						break;
					}
					continue;
				}

				if (::snd_pcm_state(pcm_.get()) == SND_PCM_STATE_RUNNING) {
					initial_fill = false;
				}

				frames_remaining -= written;
				write_ptr += static_cast<size_t>(written) * channels;
				stream_frame_ += written;

				if (frames_remaining > 0 && !WaitUntilWritable(stop_token)) {
					break;
				}
			}

			if (result != DataCallbackResult::CONTINUE) {
				break;
			}
		}
	}
	catch (const std::exception& e) {
		if (stop_requested_ || stop_token.stop_requested()) {
			XAMP_LOG_D(logger_, "ALSA render loop stopped: {}.", e.what());
			is_running_ = false;
			stop_requested_ = true;
			return;
		}

		XAMP_LOG_D(logger_, "ALSA render loop failed: {}.", e.what());
		if (callback_ != nullptr) {
			callback_->OnError(e);
		}
	}

	is_running_ = false;
	stop_requested_ = true;
}

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
