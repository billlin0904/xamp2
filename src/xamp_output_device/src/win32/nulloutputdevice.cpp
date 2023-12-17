#include <output_device/win32/nulloutputdevice.h>

#include <base/executor.h>
#include <base/ithreadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/timer.h>
#include <output_device/iaudiocallback.h>

#ifdef XAMP_OS_WIN

#include <output_device/win32/wasapi.h>
#include <output_device/win32/mmcss.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

using namespace helper;

NullOutputDevice::NullOutputDevice()
	: is_running_(false)
	, raw_mode_(false)
	, is_muted_(false)
	, is_stopped_(true)
	, volume_(0)
	, buffer_frames_(0)
	, callback_(nullptr)
	, wait_time_(0)
	, logger_(LoggerManager::GetInstance().GetLogger(kNullOutputDeviceLoggerName)) {
}

NullOutputDevice::~NullOutputDevice() = default;

bool NullOutputDevice::IsStreamOpen() const noexcept {
	return true;
}

void NullOutputDevice::SetAudioCallback(IAudioCallback* callback) noexcept {
	callback_ = callback;
}

void NullOutputDevice::StopStream(bool wait_for_stop_stream) {
	if (!is_running_) {
		return;
	}

	XAMP_LOG_DEBUG("NullOutputDevice stop stream start.");

	is_stopped_ = true;
	if (render_task_.valid()) {
		render_task_.get();
	}

	XAMP_LOG_DEBUG("NullOutputDevice stop stream done.");
	is_running_ = false;
}

void NullOutputDevice::CloseStream() {
	render_task_ = Task<void>();
}

void NullOutputDevice::OpenStream(AudioFormat const & output_format) {
	static constexpr auto kDefaultBufferFrame = 432;

	buffer_frames_ = kDefaultBufferFrame;
	const size_t buffer_size = buffer_frames_ * output_format.GetChannels();
	if (buffer_.size() != buffer_size) {
		buffer_ = MakeBuffer<float>(buffer_size);
	}
	output_format_ = output_format;

	wait_time_ = std::chrono::milliseconds((buffer_frames_ *
		kMicrosecondsPerSecond /
		output_format.GetSampleRate()) / 1000);
}

bool NullOutputDevice::IsMuted() const {
	return is_muted_;
}

uint32_t NullOutputDevice::GetVolume() const {
	return volume_;
}

void NullOutputDevice::SetMute(bool mute) const {
	is_muted_ = mute;
}

PackedFormat NullOutputDevice::GetPackedFormat() const noexcept {
	return PackedFormat::INTERLEAVED;
}

uint32_t NullOutputDevice::GetBufferSize() const noexcept {
	return buffer_frames_ * AudioFormat::kMaxChannel;
}

void NullOutputDevice::SetVolume(uint32_t volume) const {
	volume_ = std::clamp(volume, static_cast<uint32_t>(0), static_cast<uint32_t>(100));
}

void NullOutputDevice::SetStreamTime(double stream_time) noexcept {
	stream_time_ = static_cast<int64_t>(stream_time 
		* static_cast<double>(output_format_.GetSampleRate()));
}

double NullOutputDevice::GetStreamTime() const noexcept {
	return stream_time_ / static_cast<double>(output_format_.GetSampleRate());
}

void NullOutputDevice::StartStream() {
	is_stopped_ = false;
	render_task_ = Executor::Spawn(GetWasapiThreadPool(), [this](const StopToken& stop_token) {
		Mmcss mmcss;
		size_t num_filled_frames = 0;		
		double sample_time = 0;

		mmcss.BoostPriority(kMmcssProfileProAudio, MmcssThreadPriority::MMCSS_THREAD_PRIORITY_NORMAL);

		while (!is_stopped_ && !stop_token.stop_requested()) {
			const auto stream_time = stream_time_ + buffer_frames_;
			stream_time_ = stream_time;
			const auto stream_time_float = static_cast<double>(stream_time) / output_format_.GetSampleRate();

			is_running_ = true;
			XAMP_UNLIKELY(callback_->OnGetSamples(buffer_.Get(), buffer_frames_, num_filled_frames, stream_time_float, sample_time) == DataCallbackResult::CONTINUE) {
				break;
			}
			std::this_thread::sleep_for(wait_time_);
		}

		mmcss.RevertPriority();
		XAMP_LOG_DEBUG("NullOutputDevice stop render.");

		}, ExecuteFlags::EXECUTE_LONG_RUNNING);
}

bool NullOutputDevice::IsStreamRunning() const noexcept {
	return is_running_;
}

void NullOutputDevice::AbortStream() noexcept {
}

void NullOutputDevice::SetIoFormat(DsdIoFormat format) {
	if (format == DsdIoFormat::IO_FORMAT_DSD) {
		raw_mode_ = true;
	}
	else {
		raw_mode_ = false;
	}
}

DsdIoFormat NullOutputDevice::GetIoFormat() const {
	return raw_mode_ ? DsdIoFormat::IO_FORMAT_DSD : DsdIoFormat::IO_FORMAT_PCM;
}

bool NullOutputDevice::IsHardwareControlVolume() const {
	return true;
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
