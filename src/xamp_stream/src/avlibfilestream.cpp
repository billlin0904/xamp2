//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <stream/avlibfilestream.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <vector>

#include <base/archivefile.h>
#include <base/buffer.h>
#include <base/exception.h>
#include <base/logger.h>
#include <base/memory.h>
#include <base/str_utilts.h>

#include <stream/avlib.h>

XAMP_STREAM_NAMESPACE_BEGIN

namespace {

XAMP_DECLARE_LOG_NAME(AvLibFileStream);

constexpr auto kOutputSampleFormat = AV_SAMPLE_FMT_FLT;

std::string ToAvFileName(const Path& file_path) {
	return String::ToUtf8String(file_path.wstring());
}

int64_t DefaultChannelLayout(int channels) {
	switch (channels) {
	case 1:
		return AV_CH_LAYOUT_MONO;
	case 2:
		return AV_CH_LAYOUT_STEREO;
	case 3:
		return AV_CH_LAYOUT_SURROUND;
	case 4:
		return AV_CH_LAYOUT_QUAD;
	case 5:
		return AV_CH_LAYOUT_5POINT0;
	case 6:
		return AV_CH_LAYOUT_5POINT1;
	case 7:
		return AV_CH_LAYOUT_6POINT1;
	case 8:
		return AV_CH_LAYOUT_7POINT1;
	default:
		return 0;
	}
}

int GetChannelCount(const AVCodecContext* codec_context) {
	if (codec_context->ch_layout.nb_channels > 0) {
		return codec_context->ch_layout.nb_channels;
	}
	if (codec_context->channels > 0) {
		return codec_context->channels;
	}
	return 0;
}

int64_t GetChannelLayout(const AVCodecContext* codec_context) {
	if (codec_context->ch_layout.order == AV_CHANNEL_ORDER_NATIVE
		&& codec_context->ch_layout.u.mask != 0) {
		return static_cast<int64_t>(codec_context->ch_layout.u.mask);
	}
	if (codec_context->channel_layout != 0) {
		return static_cast<int64_t>(codec_context->channel_layout);
	}
	return DefaultChannelLayout(GetChannelCount(codec_context));
}

double RationalToSeconds(int64_t value, AVRational time_base) {
	if (value <= 0 || time_base.den == 0) {
		return 0.0;
	}
	return static_cast<double>(value) * static_cast<double>(time_base.num)
		/ static_cast<double>(time_base.den);
}

} // namespace

class AvLibFileStream::AvLibFileStreamImpl {
public:
	AvLibFileStreamImpl()
		: logger_(XAMP_LOG_CREATE_LOGGER(AvLibFileStream)) {
		logger_->SetLevel(LogLevel::LOG_LEVEL_DEBUG);
	}

	~AvLibFileStreamImpl() {
		Close();
	}

	void OpenFile(const Path& file_path) {
		Close();

		file_path_ = file_path;
		auto file_name = ToAvFileName(file_path);
		XAMP_LOG_D(logger_, "Open AvLib file stream start: {}.", file_name);

		AVFormatContext* raw_format_context = nullptr;
		AvIfFailedThrow(LibAvDLL.Format->avformat_open_input(
			&raw_format_context,
			file_name.c_str(),
			nullptr,
			nullptr));
		format_context_ = raw_format_context;
		XAMP_LOG_D(logger_,
			"AvLib input opened: {} format:{} streams:{}.",
			file_name,
			format_context_->iformat != nullptr ? format_context_->iformat->name : "unknown",
			format_context_->nb_streams);

		AvIfFailedThrow(LibAvDLL.Format->avformat_find_stream_info(format_context_, nullptr));
		XAMP_LOG_D(logger_,
			"AvLib stream info loaded: {} duration:{:.2f}s bit_rate:{}kbps.",
			file_name,
			format_context_->duration > 0
				? static_cast<double>(format_context_->duration) / static_cast<double>(AV_TIME_BASE)
				: 0.0,
			format_context_->bit_rate > 0
				? static_cast<uint32_t>(format_context_->bit_rate / 1000)
				: 0);

		const auto stream_index = LibAvDLL.Format->av_find_best_stream(
			format_context_,
			AVMEDIA_TYPE_AUDIO,
			-1,
			-1,
			nullptr,
			0);
		if (stream_index < 0) {
			Throw<NotSupportFormatException>("No audio stream found in {}.", file_name);
		}
		audio_stream_index_ = stream_index;
		audio_stream_ = format_context_->streams[audio_stream_index_];
		auto* codec_parameters = audio_stream_->codecpar;
		auto* decoder = LibAvDLL.Codec->avcodec_find_decoder(codec_parameters->codec_id);
		XAMP_LOG_D(logger_,
			"AvLib selected audio stream:{} codec_id:{} sample_rate:{} channels:{} sample_fmt:{}.",
			audio_stream_index_,
			static_cast<int>(codec_parameters->codec_id),
			codec_parameters->sample_rate,
			codec_parameters->ch_layout.nb_channels,
			codec_parameters->format >= 0
				? LibAvDLL.Util->av_get_sample_fmt_name(static_cast<AVSampleFormat>(codec_parameters->format))
				: "unknown");
		if (decoder == nullptr) {
			Throw<NotSupportFormatException>(
				"No FFmpeg decoder found. codec id:{} file:{}.",
				static_cast<int>(codec_parameters->codec_id),
				file_name);
		}
		XAMP_LOG_D(logger_, "AvLib decoder selected: {}.", decoder->name != nullptr ? decoder->name : "unknown");

		codec_context_.reset(LibAvDLL.Codec->avcodec_alloc_context3(decoder));
		if (!codec_context_) {
			throw std::bad_alloc();
		}
		AvIfFailedThrow(LibAvDLL.Codec->avcodec_parameters_to_context(
			codec_context_.get(),
			codec_parameters));
		AvIfFailedThrow(LibAvDLL.Codec->avcodec_open2(
			codec_context_.get(),
			decoder,
			nullptr));

		packet_.reset(LibAvDLL.Codec->av_packet_alloc());
		frame_.reset(LibAvDLL.Util->av_frame_alloc());
		if (!packet_ || !frame_) {
			throw std::bad_alloc();
		}

		output_channels_ = GetChannelCount(codec_context_.get());
		if (output_channels_ <= 0) {
			Throw<NotSupportFormatException>("Invalid audio channel count. file:{}.", file_name);
		}
		output_sample_rate_ = codec_context_->sample_rate;
		if (output_sample_rate_ <= 0) {
			Throw<NotSupportFormatException>("Invalid audio sample rate. file:{}.", file_name);
		}

		format_ = AudioFormat(DataFormat::FORMAT_PCM,
			static_cast<uint16_t>(output_channels_),
			ByteFormat::FLOAT32,
			static_cast<uint32_t>(output_sample_rate_));
		bit_depth_ = ResolveBitDepth(codec_parameters);
		bit_rate_ = codec_parameters->bit_rate > 0
			? static_cast<uint32_t>(codec_parameters->bit_rate / 1000)
			: 0;
		duration_ = ResolveDuration();
		InitializeResampler();

		active_ = true;
		eof_ = false;

		XAMP_LOG_D(logger_,
			"Open AvLib file stream: {} format:{} duration:{:.2f}s bit_depth:{} bitrate:{}kbps.",
			file_name,
			format_,
			duration_,
			bit_depth_,
			bit_rate_);
	}

	void Open(ArchiveEntry) {
		Throw<NotSupportFormatException>("AvLibFileStream does not support archive entry yet.");
	}

	void Close() {
		if (format_context_ != nullptr) {
			XAMP_LOG_D(logger_, "Close AvLib file stream: {}.", ToAvFileName(file_path_));
		}
		pending_samples_.clear();
		pending_sample_offset_ = 0;
		swr_context_.reset();
		frame_.reset();
		packet_.reset();
		codec_context_.reset();
		if (format_context_ != nullptr) {
			LibAvDLL.Format->avformat_close_input(&format_context_);
			format_context_ = nullptr;
		}
		audio_stream_ = nullptr;
		audio_stream_index_ = -1;
		output_channels_ = 0;
		output_sample_rate_ = 0;
		bit_depth_ = 0;
		bit_rate_ = 0;
		duration_ = 0.0;
		format_.Reset();
		active_ = false;
		eof_ = true;
	}

	[[nodiscard]] double GetDuration() const {
		return duration_;
	}

	[[nodiscard]] AudioFormat GetFormat() const {
		return format_;
	}

	[[nodiscard]] bool EndOfStream() const {
		return eof_;
	}

	void Seek(double stream_time) {
		if (!format_context_ || !codec_context_ || audio_stream_index_ < 0 || audio_stream_ == nullptr) {
			XAMP_LOG_D(logger_, "AvLib seek ignored because stream is not opened. target:{:.3f}s.", stream_time);
			return;
		}

		const auto target = static_cast<int64_t>(
			stream_time * static_cast<double>(audio_stream_->time_base.den)
			/ static_cast<double>(audio_stream_->time_base.num));

		auto flags = AVSEEK_FLAG_BACKWARD;
		AvIfFailedThrow(LibAvDLL.Format->av_seek_frame(
			format_context_,
			audio_stream_index_,
			target,
			flags));
		LibAvDLL.Codec->avcodec_flush_buffers(codec_context_.get());
		pending_samples_.clear();
		pending_sample_offset_ = 0;
		eof_ = false;
		active_ = true;
		if (swr_context_) {
			LibAvDLL.Swr->swr_close(swr_context_.get());
			AvIfFailedThrow(LibAvDLL.Swr->swr_init(swr_context_.get()));
		}
		XAMP_LOG_D(logger_,
			"AvLib seek completed: target:{:.3f}s timestamp:{} stream:{}.",
			stream_time,
			target,
			audio_stream_index_);
	}

	[[nodiscard]] uint32_t GetSamples(void* buffer, uint32_t length) {
		if (buffer == nullptr || length == 0 || !active_) {
			return 0;
		}

		auto* output = static_cast<float*>(buffer);
		uint32_t copied_samples = 0;

		while (copied_samples < length) {
			const auto copied_from_pending = CopyPendingSamples(
				output + copied_samples,
				length - copied_samples);
			copied_samples += copied_from_pending;
			if (copied_samples >= length) {
				break;
			}

			if (eof_) {
				active_ = HasPendingSamples();
				break;
			}

			if (!DecodeNextFrame()) {
				eof_ = true;
				XAMP_LOG_D(logger_, "AvLib reached input EOF, draining decoder.");
				DrainDecoder();
				if (!HasPendingSamples()) {
					active_ = false;
					XAMP_LOG_D(logger_, "AvLib stream drained.");
				}
			}
		}

		return copied_samples;
	}

	[[nodiscard]] uint32_t GetSampleSize() const {
		return sizeof(float);
	}

	[[nodiscard]] bool IsActive() const {
		return active_ || HasPendingSamples();
	}

	[[nodiscard]] uint32_t GetBitDepth() const {
		return bit_depth_;
	}

	[[nodiscard]] uint32_t GetBitRate() const {
		return bit_rate_;
	}

private:
	void InitializeResampler() {
		const auto input_channel_layout = GetChannelLayout(codec_context_.get());
		if (input_channel_layout == 0) {
			Throw<NotSupportFormatException>("Unsupported channel layout.");
		}

		swr_context_.reset(LibAvDLL.Swr->swr_alloc_set_opts(
			nullptr,
			input_channel_layout,
			kOutputSampleFormat,
			output_sample_rate_,
			input_channel_layout,
			codec_context_->sample_fmt,
			codec_context_->sample_rate,
			0,
			nullptr));
		if (!swr_context_) {
			throw std::bad_alloc();
		}
		AvIfFailedThrow(LibAvDLL.Swr->swr_init(swr_context_.get()));
		XAMP_LOG_D(logger_,
			"AvLib resampler ready: input:{}Hz/{}ch/{} output:{}Hz/{}ch/{} layout:0x{:X}.",
			codec_context_->sample_rate,
			GetChannelCount(codec_context_.get()),
			LibAvDLL.Util->av_get_sample_fmt_name(codec_context_->sample_fmt),
			output_sample_rate_,
			output_channels_,
			LibAvDLL.Util->av_get_sample_fmt_name(kOutputSampleFormat),
			input_channel_layout);
	}

	[[nodiscard]] uint32_t ResolveBitDepth(const AVCodecParameters* codec_parameters) const {
		auto bits = codec_parameters->bits_per_raw_sample;
		if (bits <= 0) {
			bits = codec_parameters->bits_per_coded_sample;
		}
		if (bits <= 0) {
			bits = LibAvDLL.Codec->av_get_bits_per_sample(codec_parameters->codec_id);
		}
		return bits > 0 ? static_cast<uint32_t>(bits) : format_.GetBitsPerSample();
	}

	[[nodiscard]] double ResolveDuration() const {
		if (audio_stream_ != nullptr && audio_stream_->duration > 0) {
			return RationalToSeconds(audio_stream_->duration, audio_stream_->time_base);
		}
		if (format_context_ != nullptr && format_context_->duration > 0) {
			return static_cast<double>(format_context_->duration) / static_cast<double>(AV_TIME_BASE);
		}
		return 0.0;
	}

	[[nodiscard]] bool HasPendingSamples() const {
		return pending_sample_offset_ < pending_samples_.size();
	}

	uint32_t CopyPendingSamples(float* output, uint32_t available_samples) {
		if (!HasPendingSamples()) {
			pending_samples_.clear();
			pending_sample_offset_ = 0;
			return 0;
		}

		const auto pending_count = pending_samples_.size() - pending_sample_offset_;
		const auto copy_count = (std::min)(static_cast<size_t>(available_samples), pending_count);
		MemoryCopy(output,
			pending_samples_.data() + pending_sample_offset_,
			copy_count * sizeof(float));
		pending_sample_offset_ += copy_count;
		if (!HasPendingSamples()) {
			pending_samples_.clear();
			pending_sample_offset_ = 0;
		}
		return static_cast<uint32_t>(copy_count);
	}

	bool DecodeNextFrame() {
		while (true) {
			const auto receive_result = LibAvDLL.Codec->avcodec_receive_frame(
				codec_context_.get(),
				frame_.get());
			if (receive_result == 0) {
				ConvertFrame(frame_.get());
				LibAvDLL.Util->av_frame_unref(frame_.get());
				return true;
			}
			if (receive_result != AVERROR(EAGAIN)) {
				if (receive_result == AVERROR_EOF) {
					return false;
				}
				AvIfFailedThrow(receive_result);
			}

			while (true) {
				const auto read_result = LibAvDLL.Format->av_read_frame(format_context_, packet_.get());
				if (read_result < 0) {
					XAMP_LOG_D(logger_, "AvLib av_read_frame reached EOF/error:{}.", read_result);
					const auto send_result = LibAvDLL.Codec->avcodec_send_packet(codec_context_.get(), nullptr);
					if (send_result != 0 && send_result != AVERROR_EOF) {
						AvIfFailedThrow(send_result);
					}
					return false;
				}

				if (packet_->stream_index == audio_stream_index_) {
					const auto send_result = LibAvDLL.Codec->avcodec_send_packet(
						codec_context_.get(),
						packet_.get());
					LibAvDLL.Codec->av_packet_unref(packet_.get());
					if (send_result == AVERROR(EAGAIN)) {
						break;
					}
					AvIfFailedThrow(send_result);
					break;
				}
				LibAvDLL.Codec->av_packet_unref(packet_.get());
			}
		}
	}

	void DrainDecoder() {
		while (true) {
			const auto receive_result = LibAvDLL.Codec->avcodec_receive_frame(
				codec_context_.get(),
				frame_.get());
			if (receive_result == AVERROR_EOF || receive_result == AVERROR(EAGAIN)) {
				return;
			}
			AvIfFailedThrow(receive_result);
			ConvertFrame(frame_.get());
			LibAvDLL.Util->av_frame_unref(frame_.get());
		}
	}

	void ConvertFrame(AVFrame* frame) {
		if (frame == nullptr || frame->nb_samples <= 0) {
			return;
		}

		const auto max_output_samples = LibAvDLL.Swr->swr_get_out_samples(
			swr_context_.get(),
			frame->nb_samples);
		if (max_output_samples <= 0) {
			return;
		}

		output_samples_.resize(static_cast<size_t>(max_output_samples) * static_cast<size_t>(output_channels_));
		auto* output_data = reinterpret_cast<uint8_t*>(output_samples_.data());
		auto converted_samples = LibAvDLL.Swr->swr_convert(
			swr_context_.get(),
			&output_data,
			max_output_samples,
			const_cast<const uint8_t**>(frame->extended_data),
			frame->nb_samples);
		if (converted_samples < 0) {
			AvIfFailedThrow(converted_samples);
		}

		if (converted_samples == 0) {
			return;
		}

		const auto total_samples = static_cast<size_t>(converted_samples)
			* static_cast<size_t>(output_channels_);
		const auto old_size = pending_samples_.size();
		pending_samples_.resize(old_size + total_samples);
		MemoryCopy(pending_samples_.data() + old_size,
			output_samples_.data(),
			total_samples * sizeof(float));
	}
	
	Path file_path_;
	Buffer<float> output_samples_;
	std::vector<float> pending_samples_;
	LoggerPtr logger_;
	AvPtr<AVCodecContext> codec_context_;
	AvPtr<AVPacket> packet_;
	AvPtr<AVFrame> frame_;
	AvPtr<SwrContext> swr_context_;
	AVFormatContext* format_context_{ nullptr };
	AVStream* audio_stream_{ nullptr };
	size_t pending_sample_offset_{ 0 };
	double duration_{ 0.0 };
	AudioFormat format_{ AudioFormat::kUnknownFormat };
	uint32_t bit_depth_{ 0 };
	uint32_t bit_rate_{ 0 };
	int audio_stream_index_{ -1 };
	int output_channels_{ 0 };
	int output_sample_rate_{ 0 };
	bool active_{ false };
	bool eof_{ true };
};

AvLibFileStream::AvLibFileStream()
	: impl_(MakeAlign<AvLibFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(AvLibFileStream)

void AvLibFileStream::OpenFile(const Path& file_path) {
	impl_->OpenFile(file_path);
}

void AvLibFileStream::Open(ArchiveEntry archive_entry) {
	impl_->Open(std::move(archive_entry));
}

void AvLibFileStream::Close() {
	impl_->Close();
}

double AvLibFileStream::GetDuration() const {
	return impl_->GetDuration();
}

AudioFormat AvLibFileStream::GetFormat() const {
	return impl_->GetFormat();
}

void AvLibFileStream::Seek(double stream_time) const {
	impl_->Seek(stream_time);
}

uint32_t AvLibFileStream::GetSamples(void* buffer, uint32_t length) const {
	return impl_->GetSamples(buffer, length);
}

uint32_t AvLibFileStream::GetSampleSize() const {
	return impl_->GetSampleSize();
}

bool AvLibFileStream::IsActive() const {
	return impl_->IsActive();
}

uint32_t AvLibFileStream::GetBitDepth() const {
	return impl_->GetBitDepth();
}

uint32_t AvLibFileStream::GetBitRate() const {
	return impl_->GetBitRate();
}

bool AvLibFileStream::EndOfStream() const {
	return impl_->EndOfStream();
}


XAMP_STREAM_NAMESPACE_END
