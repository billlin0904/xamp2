//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <output_device/posix/pulseoutputdevice.h>

#include <algorithm>

#include <pulse/pulseaudio.h>

#include <base/exception.h>
#include <base/logger.h>
#include <base/volume.h>

#include <output_device/iaudiocallback.h>
#include <output_device/posix/thread_priority.h>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

namespace {
constexpr uint32_t kDefaultCallbackMilliseconds = 50;
constexpr uint32_t kBufferPeriodCount = 4;
constexpr auto kPulseDefaultDeviceId = "default";
constexpr auto kPulsePlaybackStreamFlags = static_cast<pa_stream_flags_t>(
	PA_STREAM_INTERPOLATE_TIMING
	| PA_STREAM_ADJUST_LATENCY
	| PA_STREAM_AUTO_TIMING_UPDATE
	| PA_STREAM_NOT_MONOTONIC
	| PA_STREAM_START_CORKED);

uint32_t FramesFromMilliseconds(uint32_t sample_rate, uint32_t milliseconds) {
	return (std::max<uint32_t>)(1,
		static_cast<uint32_t>((static_cast<uint64_t>(sample_rate) * milliseconds + 999) / 1000));
}

double FramesToMilliseconds(uint32_t frames, uint32_t sample_rate) {
	if (sample_rate == 0) {
		return 0.0;
	}
	return static_cast<double>(frames) * 1000.0 / static_cast<double>(sample_rate);
}

pa_sample_format_t ToPulseSampleFormat(const AudioFormat& format) {
	if (format.GetFormat() != DataFormat::FORMAT_PCM) {
		throw DeviceUnSupportedFormatException(format);
	}

	if (format.GetPackedFormat() != PackedFormat::INTERLEAVED) {
		throw DeviceUnSupportedFormatException(format);
	}

	switch (format.GetByteFormat()) {
	case ByteFormat::FLOAT32:
		return PA_SAMPLE_FLOAT32LE;
	default:
		throw DeviceUnSupportedFormatException(format);
	}
}

void ThrowPulseError(std::string_view operation, pa_context* context) {
	const auto error = context != nullptr ? ::pa_context_errno(context) : PA_ERR_UNKNOWN;
	Throw<PlatformException>("PulseAudio {} failed: {}", operation, ::pa_strerror(error));
}

void ThrowPulseStreamError(std::string_view operation, pa_stream* stream) {
	const auto* context = stream != nullptr ? ::pa_stream_get_context(stream) : nullptr;
	const auto error = context != nullptr ? ::pa_context_errno(context) : PA_ERR_UNKNOWN;
	Throw<PlatformException>("PulseAudio {} failed: {}", operation, ::pa_strerror(error));
}
}

PulseOutputDevice::PulseOutputDevice(const std::shared_ptr<xamp::base::IThreadPoolExecutor>& thread_pool,
	std::string device_id)
	: device_id_(std::move(device_id))
	, logger_(XampLoggerFactory.GetLogger(XAMP_LOG_NAME(PulseOutputDevice))) {
	(void)thread_pool;
	if (device_id_.empty() || device_id_ == kPulseDefaultDeviceId) {
		device_id_.clear();
	}
	logger_->SetLevel(LogLevel::LOG_LEVEL_DEBUG);
}

PulseOutputDevice::~PulseOutputDevice() {
	try {
		CloseStream();
	}
	catch (...) {
	}
}

void PulseOutputDevice::ContextStateCallback(pa_context*, void* userdata) {
	auto* self = static_cast<PulseOutputDevice*>(userdata);
	if (self != nullptr && self->mainloop_ != nullptr) {
		::pa_threaded_mainloop_signal(self->mainloop_.get(), 0);
	}
}

void PulseOutputDevice::StreamStateCallback(pa_stream*, void* userdata) {
	auto* self = static_cast<PulseOutputDevice*>(userdata);
	if (self != nullptr && self->mainloop_ != nullptr) {
		::pa_threaded_mainloop_signal(self->mainloop_.get(), 0);
	}
}

void PulseOutputDevice::StreamWriteCallback(pa_stream* stream, size_t bytes, void* userdata) {
	auto* self = static_cast<PulseOutputDevice*>(userdata);
	if (self == nullptr) {
		return;
	}
	self->ConfigureRealtimeThreadPriority();
	self->OnStreamWrite(stream, bytes);
}

void PulseOutputDevice::StreamSuccessCallback(pa_stream*, int, void* userdata) {
	auto* self = static_cast<PulseOutputDevice*>(userdata);
	if (self != nullptr && self->mainloop_ != nullptr) {
		::pa_threaded_mainloop_signal(self->mainloop_.get(), 0);
	}
}

void PulseOutputDevice::WaitForContextReady() const {
	while (true) {
		switch (::pa_context_get_state(context_.get())) {
		case PA_CONTEXT_READY:
			return;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			ThrowPulseError("connect", context_.get());
		default:
			::pa_threaded_mainloop_wait(mainloop_.get());
			break;
		}
	}
}

void PulseOutputDevice::WaitForStreamReady() const {
	while (true) {
		switch (::pa_stream_get_state(stream_.get())) {
		case PA_STREAM_READY:
			return;
		case PA_STREAM_FAILED:
		case PA_STREAM_TERMINATED:
			ThrowPulseStreamError("stream connect", stream_.get());
		default:
			::pa_threaded_mainloop_wait(mainloop_.get());
			break;
		}
	}
}

void PulseOutputDevice::WaitForOperation(pa_operation* operation) const {
	PulseOperationPtr operation_ptr(operation);
	if (operation_ptr == nullptr) {
		Throw<PlatformException>("PulseAudio operation create failed.");
	}

	while (::pa_operation_get_state(operation_ptr.get()) == PA_OPERATION_RUNNING) {
		::pa_threaded_mainloop_wait(mainloop_.get());
	}
}

void PulseOutputDevice::OpenStream(const AudioFormat& output_format) {
	CloseStream();

	XAMP_LOG_D(logger_, "PulseOutputDevice open stream: {}.", output_format.ToString());

	const pa_sample_spec sample_spec{
		.format = ToPulseSampleFormat(output_format),
		.rate = output_format.GetSampleRate(),
		.channels = static_cast<uint8_t>(output_format.GetChannels()),
	};

	if (::pa_sample_spec_valid(&sample_spec) == 0) {
		throw DeviceUnSupportedFormatException(output_format);
	}

	buffer_frames_ = FramesFromMilliseconds(output_format.GetSampleRate(), kDefaultCallbackMilliseconds);
	const auto buffer_bytes = buffer_frames_ * output_format.GetBlockAlign();
	const auto target_buffer_bytes = buffer_bytes * kBufferPeriodCount;
	const pa_buffer_attr buffer_attr{
		.maxlength = static_cast<uint32_t>(target_buffer_bytes),
		.tlength = static_cast<uint32_t>(target_buffer_bytes),
		.prebuf = static_cast<uint32_t>(buffer_bytes),
		.minreq = static_cast<uint32_t>(buffer_bytes),
		.fragsize = static_cast<uint32_t>(-1),
	};

	try {
		mainloop_.reset(::pa_threaded_mainloop_new());
		if (mainloop_ == nullptr) {
			Throw<PlatformException>("PulseAudio mainloop create failed.");
		}

		context_.reset(::pa_context_new(::pa_threaded_mainloop_get_api(mainloop_.get()), "XAMP"));
		if (context_ == nullptr) {
			Throw<PlatformException>("PulseAudio context create failed.");
		}
		::pa_context_set_state_callback(context_.get(), ContextStateCallback, this);

		if (::pa_threaded_mainloop_start(mainloop_.get()) < 0) {
			Throw<PlatformException>("PulseAudio mainloop start failed.");
		}

		const PulseThreadedMainloopLock lock(mainloop_.get());

		if (::pa_context_connect(context_.get(), nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
			ThrowPulseError("connect", context_.get());
		}
		WaitForContextReady();

		stream_.reset(::pa_stream_new(context_.get(), "Playback", &sample_spec, nullptr));
		if (stream_ == nullptr) {
			ThrowPulseError("stream create", context_.get());
		}

		::pa_stream_set_state_callback(stream_.get(), StreamStateCallback, this);
		::pa_stream_set_write_callback(stream_.get(), StreamWriteCallback, this);

		if (::pa_stream_connect_playback(stream_.get(),
			device_id_.empty() ? nullptr : device_id_.c_str(),
			&buffer_attr,
			kPulsePlaybackStreamFlags,
			nullptr,
			nullptr) < 0) {
			ThrowPulseStreamError("stream connect", stream_.get());
		}
		WaitForStreamReady();

		const auto* actual_attr = ::pa_stream_get_buffer_attr(stream_.get());
		if (actual_attr != nullptr) {
			const auto device_name = device_id_.empty() ? kPulseDefaultDeviceId : device_id_;
			XAMP_LOG_D(logger_,
				"PulseAudio stream ready device:{} callback_frames:{}({:.2f}ms) tlength:{} maxlength:{} minreq:{} prebuf:{}.",
				device_name,
				buffer_frames_,
				FramesToMilliseconds(buffer_frames_, output_format.GetSampleRate()),
				actual_attr->tlength,
				actual_attr->maxlength,
				actual_attr->minreq,
				actual_attr->prebuf);
		}
	}
	catch (...) {
		CloseStream();
		throw;
	}

	output_format_ = output_format;
	is_stopped_ = true;
	is_running_ = false;
}

void PulseOutputDevice::SetAudioCallback(IAudioCallback* callback) {
	callback_ = callback;
}

bool PulseOutputDevice::IsStreamOpen() const {
	return stream_ != nullptr;
}

bool PulseOutputDevice::IsStreamRunning() const {
	return is_running_;
}

void PulseOutputDevice::StopStream(bool wait_for_stop_stream) {
	if (!IsStreamOpen()) {
		return;
	}

	is_stopped_ = true;
	is_running_ = false;

	const PulseThreadedMainloopLock lock(mainloop_.get());

	if (stream_ != nullptr) {
		::pa_stream_set_write_callback(stream_.get(), nullptr, nullptr);
		if (auto* cork = ::pa_stream_cork(stream_.get(), 1, StreamSuccessCallback, this)) {
			if (wait_for_stop_stream) {
				WaitForOperation(cork);
			}
			else {
				::pa_operation_unref(cork);
			}
		}
		if (auto* flush = ::pa_stream_flush(stream_.get(), StreamSuccessCallback, this)) {
			if (wait_for_stop_stream) {
				WaitForOperation(flush);
			}
			else {
				::pa_operation_unref(flush);
			}
		}
	}
}

void PulseOutputDevice::CloseStream() {
	if (mainloop_ == nullptr) {
		return;
	}

	is_stopped_ = true;
	is_running_ = false;

	{
		const PulseThreadedMainloopLock lock(mainloop_.get());

		if (stream_ != nullptr) {
			::pa_stream_set_write_callback(stream_.get(), nullptr, nullptr);
			::pa_stream_set_state_callback(stream_.get(), nullptr, nullptr);
			stream_.reset();
		}
		if (context_ != nullptr) {
			::pa_context_set_state_callback(context_.get(), nullptr, nullptr);
			context_.reset();
		}
	}

	::pa_threaded_mainloop_stop(mainloop_.get());
	mainloop_.reset();
}

void PulseOutputDevice::StartStream() {
	if (!IsStreamOpen() || is_running_) {
		return;
	}

	if (callback_ == nullptr) {
		Throw<PlatformException>("PulseAudio callback is not set.");
	}

	is_stopped_ = false;
	is_running_ = true;

	const PulseThreadedMainloopLock lock(mainloop_.get());

	try {
		::pa_stream_set_write_callback(stream_.get(), StreamWriteCallback, this);
		WaitForOperation(::pa_stream_cork(stream_.get(), 0, StreamSuccessCallback, this));
		if (auto* trigger = ::pa_stream_trigger(stream_.get(), StreamSuccessCallback, this)) {
			WaitForOperation(trigger);
		}
		ResetPulseTimeBase();

		const auto writable_size = ::pa_stream_writable_size(stream_.get());
		if (writable_size == static_cast<size_t>(-1)) {
			ThrowPulseStreamError("get writable size", stream_.get());
		}
		if (writable_size > 0) {
			XAMP_LOG_D(logger_, "PulseAudio prime writable bytes:{}.", writable_size);
			OnStreamWrite(stream_.get(), writable_size);
		}
	}
	catch (...) {
		if (stream_ != nullptr) {
			::pa_stream_set_write_callback(stream_.get(), nullptr, nullptr);
		}
		is_stopped_ = true;
		is_running_ = false;
		throw;
	}
}

void PulseOutputDevice::SetStreamTime(double stream_time) {
	const auto frame = static_cast<int64_t>(stream_time * output_format_.GetSampleRate());
	stream_frame_ = frame;
	stream_time_offset_frame_ = frame;
	pulse_time_base_usec_ = -1;
}

double PulseOutputDevice::GetStreamTime() const {
	if (output_format_.GetSampleRate() == 0) {
		return 0;
	}
	return static_cast<double>(stream_frame_.load()) / output_format_.GetSampleRate();
}

uint32_t PulseOutputDevice::GetVolume() const {
	return volume_;
}

bool PulseOutputDevice::IsMuted() const {
	return is_muted_;
}

void PulseOutputDevice::SetVolume(uint32_t volume) const {
	volume_ = std::clamp(volume, 0U, 100U);
}

void PulseOutputDevice::SetMute(bool mute) const {
	is_muted_ = mute;
}

PackedFormat PulseOutputDevice::GetPackedFormat() const {
	return PackedFormat::INTERLEAVED;
}

uint32_t PulseOutputDevice::GetBufferSize() const {
	return buffer_frames_ * AudioFormat::kMaxChannel;
}

bool PulseOutputDevice::IsHardwareControlVolume() const {
	return false;
}

void PulseOutputDevice::ResetPulseTimeBase() {
	pa_usec_t pulse_time = 0;
	if (stream_ != nullptr && ::pa_stream_get_time(stream_.get(), &pulse_time) >= 0) {
		pulse_time_base_usec_ = static_cast<int64_t>(pulse_time);
		return;
	}
	pulse_time_base_usec_ = -1;
}

double PulseOutputDevice::GetCallbackStreamTime(int64_t fallback_frame) const {
	const auto sample_rate = output_format_.GetSampleRate();
	if (sample_rate == 0) {
		return 0;
	}

	pa_usec_t pulse_time = 0;
	const auto pulse_time_base = pulse_time_base_usec_.load();
	if (stream_ != nullptr
		&& pulse_time_base >= 0
		&& ::pa_stream_get_time(stream_.get(), &pulse_time) >= 0
		&& static_cast<int64_t>(pulse_time) >= pulse_time_base) {
		const auto offset_frame = stream_time_offset_frame_.load();
		const auto offset_time = static_cast<double>(offset_frame) / sample_rate;
		const auto elapsed_time = static_cast<double>(static_cast<int64_t>(pulse_time) - pulse_time_base)
			/ 1000000.0;
		const auto clock_time = offset_time + elapsed_time;
		const auto submitted_time = static_cast<double>(fallback_frame) / sample_rate;
		return std::min(clock_time, submitted_time);
	}

	return static_cast<double>(fallback_frame) / sample_rate;
}

void PulseOutputDevice::ConfigureRealtimeThreadPriority() {
	bool expected = false;
	if (!realtime_priority_configured_.compare_exchange_strong(expected, true)) {
		return;
	}

	SetRealtimeThreadPriority("PulseAudio callback thread");
}

void PulseOutputDevice::AbortStream() {
	if (!IsStreamOpen()) {
		return;
	}

	is_stopped_ = true;
	is_running_ = false;

	const PulseThreadedMainloopLock lock(mainloop_.get());

	if (stream_ != nullptr) {
		::pa_stream_set_write_callback(stream_.get(), nullptr, nullptr);
		if (auto* flush = ::pa_stream_flush(stream_.get(), StreamSuccessCallback, this)) {
			WaitForOperation(flush);
		}
	}
}

void PulseOutputDevice::ApplySoftwareVolume(float* samples, size_t sample_count) const {
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

void PulseOutputDevice::OnStreamWrite(pa_stream* stream, size_t bytes) {
	bool write_pending = false;
	try {
		if (is_stopped_ || callback_ == nullptr || bytes == 0) {
			return;
		}

		const auto block_align = output_format_.GetBlockAlign();
		if (block_align == 0) {
			return;
		}

		auto frames_to_write = std::min<size_t>(bytes / block_align, buffer_frames_);
		if (frames_to_write == 0) {
			return;
		}

		void* data = nullptr;
		auto bytes_to_write = frames_to_write * block_align;
		if (::pa_stream_begin_write(stream, &data, &bytes_to_write) < 0 || data == nullptr) {
			ThrowPulseStreamError("begin write", stream);
		}
		write_pending = true;

		frames_to_write = bytes_to_write / block_align;
		if (frames_to_write == 0) {
			(void)::pa_stream_cancel_write(stream);
			return;
		}

		auto* const samples = static_cast<float*>(data);
		std::fill_n(samples, frames_to_write * output_format_.GetChannels(), 0.0f);

		size_t filled_frames = 0;
		const auto current_frame = stream_frame_.load();
		const auto next_frame = current_frame + static_cast<int64_t>(frames_to_write);
		const auto stream_time = GetCallbackStreamTime(next_frame);

		const auto result = callback_->OnGetSamples(samples,
			frames_to_write,
			filled_frames,
			stream_time,
			0);

		if (result != DataCallbackResult::CONTINUE && filled_frames == 0) {
			XAMP_LOG_D(logger_,
				"PulseAudio callback stop: stream_time:{:.3f} requested:{} filled:{}.",
				stream_time,
				frames_to_write,
				filled_frames);
			(void)::pa_stream_cancel_write(stream);
			is_stopped_ = true;
			is_running_ = false;
			::pa_stream_set_write_callback(stream, nullptr, nullptr);
			return;
		}

		filled_frames = std::min(filled_frames, frames_to_write);
		if (filled_frames != frames_to_write) {
			XAMP_LOG_D(logger_,
				"PulseAudio underrun: requested:{} filled:{}. Fill remaining frames with silence.",
				frames_to_write,
				filled_frames);
		}

		const auto samples_to_write = frames_to_write * output_format_.GetChannels();
		ApplySoftwareVolume(samples, samples_to_write);

		bytes_to_write = frames_to_write * block_align;
		if (::pa_stream_write(stream, data, bytes_to_write, nullptr, 0, PA_SEEK_RELATIVE) < 0) {
			ThrowPulseStreamError("write", stream);
		}
		write_pending = false;

		stream_frame_ = current_frame + static_cast<int64_t>(frames_to_write);
	}
	catch (const std::exception& e) {
		if (write_pending) {
			(void)::pa_stream_cancel_write(stream);
		}
		is_stopped_ = true;
		is_running_ = false;
		::pa_stream_set_write_callback(stream, nullptr, nullptr);
		if (callback_ != nullptr) {
			callback_->OnError(e);
		}
	}
}

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
