//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <output_device/posix/pipewireoutputdevice.h>

#include <algorithm>
#include <array>

#include <pipewire/keys.h>
#include <spa/param/audio/format-utils.h>
#include <spa/pod/builder.h>
#include <spa/utils/result.h>

#include <base/exception.h>
#include <base/volume.h>

#include <output_device/iaudiocallback.h>
#include <output_device/posix/thread_priority.h>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

namespace {
constexpr uint32_t kDefaultCallbackMilliseconds = 50;
constexpr auto kPipeWireDefaultDeviceId = "default";

uint32_t FramesFromMilliseconds(uint32_t sample_rate, uint32_t milliseconds) {
	return (std::max<uint32_t>)(1,
		static_cast<uint32_t>((static_cast<uint64_t>(sample_rate) * milliseconds + 999) / 1000));
}

spa_audio_format ToSpaAudioFormat(const AudioFormat& format) {
	if (format.GetFormat() != DataFormat::FORMAT_PCM
		|| format.GetPackedFormat() != PackedFormat::INTERLEAVED) {
		throw DeviceUnSupportedFormatException(format);
	}

	switch (format.GetByteFormat()) {
	case ByteFormat::FLOAT32:
		return SPA_AUDIO_FORMAT_F32;
	default:
		throw DeviceUnSupportedFormatException(format);
	}
}

std::string PipeWireErrorToString(int error) {
	const char* error_str = spa_strerror(error);
	return error_str != nullptr ? String::Format("{} ((})", error, error_str): "unknown";
}

#define PipeWireLogIfError(expr) \
	do { \
		auto error = (expr); \
		if (error < 0) { \
			XAMP_LOG_D(logger_, "PipeWire error: {} {}", #expr, PipeWireErrorToString(error)); \
		} \
	} while (0)

}

PipeWireOutputDevice::PipeWireOutputDevice(const std::shared_ptr<xamp::base::IThreadPoolExecutor>& thread_pool,
	std::string device_id)
	: device_id_(std::move(device_id))
	, logger_(XampLoggerFactory.GetLogger(XAMP_LOG_NAME(PipeWireOutputDevice))) {
	(void)thread_pool;
	logger_->SetLevel(LogLevel::LOG_LEVEL_DEBUG);
}

PipeWireOutputDevice::~PipeWireOutputDevice() {
	try {
		CloseStream();
	}
	catch (...) {
	}
}

void PipeWireOutputDevice::CoreDoneCallback(void* userdata, uint32_t id, int seq) {
	auto* self = static_cast<PipeWireOutputDevice*>(userdata);
	if (self == nullptr || id != PW_ID_CORE || seq != self->core_sync_seq_) {
		return;
	}
	self->core_ready_ = true;
	if (self->loop_ != nullptr) {
		::pw_thread_loop_signal(self->loop_.get(), false);
	}
}

void PipeWireOutputDevice::StreamStateCallback(void* userdata,
	pw_stream_state,
	pw_stream_state state,
	const char* error) {
	auto* self = static_cast<PipeWireOutputDevice*>(userdata);
	if (self == nullptr) {
		return;
	}

	if (state == PW_STREAM_STATE_PAUSED || state == PW_STREAM_STATE_STREAMING) {
		self->stream_ready_ = true;
	}
	else if (state == PW_STREAM_STATE_ERROR) {
		XAMP_LOG_D(self->logger_, "PipeWire stream entered error state: {}.",
			error != nullptr ? error : "unknown");
		self->stream_ready_ = false;
	}

	if (self->loop_ != nullptr) {
		::pw_thread_loop_signal(self->loop_.get(), false);
	}
}

void PipeWireOutputDevice::StreamProcessCallback(void* userdata) {
	auto* self = static_cast<PipeWireOutputDevice*>(userdata);
	if (self != nullptr) {
		self->ConfigureRealtimeThreadPriority();
		self->OnStreamProcess();
	}
}

void PipeWireOutputDevice::WaitForCoreReady() {
	while (!core_ready_) {
		::pw_thread_loop_wait(loop_.get());
	}
}

void PipeWireOutputDevice::WaitForStreamReady() {
	while (!stream_ready_) {
		const auto state = pw_stream_get_state(stream_.get(), nullptr);
		if (state == PW_STREAM_STATE_ERROR || state == PW_STREAM_STATE_UNCONNECTED) {
			Throw<PlatformException>("PipeWire stream connect failed.");
		}
		::pw_thread_loop_wait(loop_.get());
	}
}

void PipeWireOutputDevice::OpenStream(const AudioFormat& output_format) {
	if (device_id_.empty()) {
		throw DeviceNotFoundException(device_id_);
	}

	CloseStream();

	XAMP_LOG_D(logger_, "PipeWireOutputDevice open stream: {}.", output_format.ToString());

	const auto spa_format = ToSpaAudioFormat(output_format);
	buffer_frames_ = FramesFromMilliseconds(output_format.GetSampleRate(), kDefaultCallbackMilliseconds);
	core_ready_ = false;
	stream_ready_ = false;

	EnsurePipeWireInitialized();

	loop_.reset(::pw_thread_loop_new("xamp-pipewire-output", nullptr));
	if (loop_ == nullptr) {
		Throw<PlatformException>("PipeWire thread loop create failed.");
	}

	context_.reset(::pw_context_new(::pw_thread_loop_get_loop(loop_.get()), nullptr, 0));
	if (context_ == nullptr) {
		Throw<PlatformException>("PipeWire context create failed.");
	}

	core_.reset(::pw_context_connect(context_.get(), nullptr, 0));
	if (core_ == nullptr) {
		Throw<PlatformException>("PipeWire core connect failed.");
	}

	static constexpr pw_core_events core_events{
		.version = PW_VERSION_CORE_EVENTS,
		.done = &PipeWireOutputDevice::CoreDoneCallback,
	};
	pw_core_add_listener(core_.get(), &core_listener_, &core_events, this);
	core_sync_seq_ = pw_core_sync(core_.get(), PW_ID_CORE, 0);

	if (::pw_thread_loop_start(loop_.get()) != 0) {
		Throw<PlatformException>("PipeWire thread loop start failed.");
	}

	{
		const PipeWireThreadLoopLock lock(loop_.get());
		WaitForCoreReady();

		auto* props = ::pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
			PW_KEY_MEDIA_CATEGORY, "Playback",
			PW_KEY_MEDIA_ROLE, "Music",
			PW_KEY_APP_ID, "xamp",
			PW_KEY_APP_NAME, "XAMP",
			nullptr);
		if (props == nullptr) {
			Throw<PlatformException>("PipeWire stream properties create failed.");
		}

		::pw_properties_setf(props, PW_KEY_NODE_RATE, "1/%u", output_format.GetSampleRate());
		::pw_properties_setf(props, PW_KEY_NODE_LATENCY, "%u/%u", buffer_frames_, output_format.GetSampleRate());
		if (!device_id_.empty()) {
			::pw_properties_setf(props, PW_KEY_TARGET_OBJECT, "%s", device_id_.c_str());
		}

		stream_.reset(::pw_stream_new(core_.get(), "Playback", props));
		if (stream_ == nullptr) {
			Throw<PlatformException>("PipeWire stream create failed.");
		}

		static constexpr pw_stream_events stream_events{
			.version = PW_VERSION_STREAM_EVENTS,
			.state_changed = &PipeWireOutputDevice::StreamStateCallback,
			.process = &PipeWireOutputDevice::StreamProcessCallback,
		};
		::pw_stream_add_listener(stream_.get(), &stream_listener_, &stream_events, this);

		std::array<uint8_t, 1024> params_buffer{};
		auto builder = SPA_POD_BUILDER_INIT(params_buffer.data(), params_buffer.size());
		spa_audio_info_raw audio_info{
			.format = spa_format,
			.flags = SPA_AUDIO_FLAG_NONE,
			.rate = output_format.GetSampleRate(),
			.channels = output_format.GetChannels(),
		};

		const spa_pod* params[]{
			::spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &audio_info),
		};

		static constexpr auto stream_flags = static_cast<pw_stream_flags>(
			PW_STREAM_FLAG_AUTOCONNECT
			| PW_STREAM_FLAG_INACTIVE
			| PW_STREAM_FLAG_MAP_BUFFERS
			| PW_STREAM_FLAG_RT_PROCESS);

		if (::pw_stream_connect(stream_.get(),
			PW_DIRECTION_OUTPUT,
			PW_ID_ANY,
			stream_flags,
			params,
			std::size(params)) < 0) {
			Throw<PlatformException>("PipeWire stream connect failed.");
		}

		WaitForStreamReady();
	}

	output_format_ = output_format;
	is_stopped_ = true;
	is_running_ = false;

	XAMP_LOG_D(logger_,
		"PipeWire stream ready device:{} callback_frames:{}({:.2f}ms).",
		device_id_,
		buffer_frames_,
		static_cast<double>(buffer_frames_) * 1000.0 / output_format.GetSampleRate());
}

void PipeWireOutputDevice::SetAudioCallback(IAudioCallback* callback) {
	callback_ = callback;
}

bool PipeWireOutputDevice::IsStreamOpen() const {
	return stream_ != nullptr;
}

bool PipeWireOutputDevice::IsStreamRunning() const {
	return is_running_;
}

void PipeWireOutputDevice::StopStream(bool) {
	if (!IsStreamOpen()) {
		return;
	}

	is_stopped_ = true;
	is_running_ = false;

	const PipeWireThreadLoopLock lock(loop_.get());
	::pw_stream_set_active(stream_.get(), false);
	::pw_stream_flush(stream_.get(), false);
}

void PipeWireOutputDevice::CloseStream() {
	if (loop_ != nullptr) {
		{
			const PipeWireThreadLoopLock lock(loop_.get());
			if (stream_ != nullptr) {
				::spa_hook_remove(&stream_listener_);
				stream_.reset();
			}
			if (core_ != nullptr) {
				::spa_hook_remove(&core_listener_);
			}
			core_.reset();
			context_.reset();
		}
		::pw_thread_loop_stop(loop_.get());
		loop_.reset();
	}
	else {
		stream_.reset();
		core_.reset();
		context_.reset();
	}

	is_stopped_ = true;
	is_running_ = false;
}

void PipeWireOutputDevice::StartStream() {
	if (!IsStreamOpen() || is_running_) {
		return;
	}
	if (callback_ == nullptr) {
		Throw<PlatformException>("PipeWire callback is not set.");
	}

	is_stopped_ = false;
	is_running_ = true;

	const PipeWireThreadLoopLock lock(loop_.get());
	::pw_stream_set_active(stream_.get(), true);
}

void PipeWireOutputDevice::SetStreamTime(double stream_time) {
	const auto frame = static_cast<int64_t>(stream_time * output_format_.GetSampleRate());
	stream_frame_ = frame;
	stream_time_offset_frame_ = frame;
}

double PipeWireOutputDevice::GetStreamTime() const {
	if (output_format_.GetSampleRate() == 0) {
		return 0.0;
	}
	return static_cast<double>(stream_frame_.load()) / output_format_.GetSampleRate();
}

uint32_t PipeWireOutputDevice::GetVolume() const {
	return volume_;
}

void PipeWireOutputDevice::SetVolume(uint32_t volume) const {
	volume_ = std::clamp(volume, 0U, 100U);
}

void PipeWireOutputDevice::SetMute(bool mute) const {
	is_muted_ = mute;
}

bool PipeWireOutputDevice::IsMuted() const {
	return is_muted_;
}

bool PipeWireOutputDevice::IsHardwareControlVolume() const {
	return false;
}

PackedFormat PipeWireOutputDevice::GetPackedFormat() const {
	return PackedFormat::INTERLEAVED;
}

uint32_t PipeWireOutputDevice::GetBufferSize() const {
	return buffer_frames_ * AudioFormat::kMaxChannel;
}

void PipeWireOutputDevice::AbortStream() {
	if (!IsStreamOpen()) {
		return;
	}

	is_stopped_ = true;
	is_running_ = false;

	const PipeWireThreadLoopLock lock(loop_.get());
	PipeWireLogIfError(::pw_stream_set_active(stream_.get(), false));
	PipeWireLogIfError(::pw_stream_flush(stream_.get(), false));
}

void PipeWireOutputDevice::ApplySoftwareVolume(float* samples, size_t sample_count) const {
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

void PipeWireOutputDevice::ConfigureRealtimeThreadPriority() {
	bool expected = false;
	if (!realtime_priority_configured_.compare_exchange_strong(expected, true)) {
		return;
	}

	SetRealtimeThreadPriority("PipeWire process thread");
}

double PipeWireOutputDevice::GetCallbackStreamTime(int64_t next_frame) const {
	const auto sample_rate = output_format_.GetSampleRate();
	if (sample_rate == 0) {
		return 0.0;
	}
	return static_cast<double>(next_frame) / sample_rate;
}

void PipeWireOutputDevice::OnStreamProcess() {
	try {
		if (is_stopped_ || callback_ == nullptr || stream_ == nullptr) {
			return;
		}

		auto* buffer = ::pw_stream_dequeue_buffer(stream_.get());
		if (buffer == nullptr || buffer->buffer == nullptr || buffer->buffer->n_datas == 0) {
			return;
		}

		auto& data = buffer->buffer->datas[0];
		if (data.data == nullptr || data.maxsize == 0 || data.chunk == nullptr) {
			::pw_stream_queue_buffer(stream_.get(), buffer);
			return;
		}

		const auto block_align = output_format_.GetBlockAlign();
		if (block_align == 0) {
			::pw_stream_queue_buffer(stream_.get(), buffer);
			return;
		}

		auto frames_to_write = (std::min<size_t>)(data.maxsize / block_align, buffer_frames_);
		if (frames_to_write == 0) {
			data.chunk->offset = 0;
			data.chunk->stride = block_align;
			data.chunk->size = 0;
			::pw_stream_queue_buffer(stream_.get(), buffer);
			return;
		}

		auto* const samples = static_cast<float*>(data.data);
		std::fill_n(samples, frames_to_write * output_format_.GetChannels(), 0.0f);

		size_t filled_frames = 0;
		const auto current_frame = stream_frame_.load();
		const auto next_frame = current_frame + static_cast<int64_t>(frames_to_write);
		const auto stream_time = GetCallbackStreamTime(next_frame);

		const auto result = callback_->OnGetSamples(samples,
			frames_to_write,
			filled_frames,
			stream_time,
			GetStreamTime());

		filled_frames = (std::min)(filled_frames, frames_to_write);
		if (result != DataCallbackResult::CONTINUE && filled_frames == 0) {
			data.chunk->offset = 0;
			data.chunk->stride = block_align;
			data.chunk->size = 0;
			::pw_stream_queue_buffer(stream_.get(), buffer);
			is_stopped_ = true;
			is_running_ = false;
			return;
		}

		if (filled_frames == 0 && result == DataCallbackResult::CONTINUE) {
			filled_frames = frames_to_write;
		}

		const auto frames_ready = result == DataCallbackResult::CONTINUE
			? frames_to_write
			: filled_frames;

		ApplySoftwareVolume(samples, frames_ready * output_format_.GetChannels());

		/*XAMP_LOG_D(logger_,
			"Stream process callback: result: {}, requested frames: {}, filled frames: {}, stream time: {:.2f}ms.",
			result == DataCallbackResult::CONTINUE ? "CONTINUE" : "STOP",
			static_cast<uint32_t>(frames_to_write),
			static_cast<uint32_t>(filled_frames),
			stream_time * 1000.0);*/

		data.chunk->offset = 0;
		data.chunk->stride = block_align;
		data.chunk->size = static_cast<uint32_t>(frames_ready * block_align);
		PipeWireLogIfError(::pw_stream_queue_buffer(stream_.get(), buffer));

		stream_frame_ = current_frame + static_cast<int64_t>(frames_ready);

		if (result != DataCallbackResult::CONTINUE) {
			is_stopped_ = true;
			is_running_ = false;
		}
	}
	catch (const std::exception& e) {
		is_stopped_ = true;
		is_running_ = false;
		if (callback_ != nullptr) {
			callback_->OnError(e);
		}
	}
}

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
