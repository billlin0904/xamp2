//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <stream/avlibfilestream.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <vector>

#include <base/archivefile.h>
#include <base/buffer.h>
#include <base/exception.h>
#include <base/fastiostream.h>
#include <base/logger.h>
#include <base/memory.h>
#include <base/str_utilts.h>

#include <stream/avlib.h>

XAMP_STREAM_NAMESPACE_BEGIN

namespace {

XAMP_DECLARE_LOG_NAME(AvLibFileStream);

constexpr auto kOutputSampleFormat = AV_SAMPLE_FMT_FLT;
constexpr auto kAvIOBufferSize = 256 * 1024;

std::string toAvFileName(const Path& file_path) {
	return String::toUtf8String(file_path.wstring());
}

int64_t defaultChannelLayout(int channels) {
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

int getChannelCount(const AVCodecContext* codec_context) {
	if (codec_context->ch_layout.nb_channels > 0) {
		return codec_context->ch_layout.nb_channels;
	}
	if (codec_context->channels > 0) {
		return codec_context->channels;
	}
	return 0;
}

int64_t getChannelLayout(const AVCodecContext* codec_context) {
	if (codec_context->ch_layout.order == AV_CHANNEL_ORDER_NATIVE
		&& codec_context->ch_layout.u.mask != 0) {
		return static_cast<int64_t>(codec_context->ch_layout.u.mask);
	}
	if (codec_context->channel_layout != 0) {
		return static_cast<int64_t>(codec_context->channel_layout);
	}
	return defaultChannelLayout(getChannelCount(codec_context));
}

struct AvFastIOContext {
	explicit AvFastIOContext(const Path& file_path)
		: stream(file_path) {
	}

	static int readPacket(void* opaque, uint8_t* buffer, int buffer_size) noexcept {
		try {
			auto* context = static_cast<AvFastIOContext*>(opaque);
			const auto bytes_read = context->stream.read(buffer, static_cast<size_t>(buffer_size));
			if (bytes_read == 0) {
				return AVERROR_EOF;
			}
			return static_cast<int>(bytes_read);
		}
		catch (...) {
			return AVERROR(EIO);
		}
	}

	static int64_t seek(void* opaque, int64_t offset, int whence) noexcept {
		try {
			auto* context = static_cast<AvFastIOContext*>(opaque);
			if (whence == AVSEEK_SIZE) {
				return static_cast<int64_t>(context->stream.size());
			}

			const auto seek_whence = whence & ~AVSEEK_FORCE;
			if (seek_whence != SEEK_SET && seek_whence != SEEK_CUR && seek_whence != SEEK_END) {
				return AVERROR(EINVAL);
			}

			context->stream.seek(offset, seek_whence);
			return static_cast<int64_t>(context->stream.tell());
		}
		catch (...) {
			return AVERROR(EIO);
		}
	}

	FastIOStream stream;
};

double rationalToSeconds(int64_t value, AVRational time_base) {
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
		//logger_->setLevel(LogLevel::LOG_LEVEL_DEBUG);
	}

	~AvLibFileStreamImpl() {
		close();
	}

	void useCustomIOContext(bool enable) {
		use_custom_io_context_ = enable;
	}

	void openFile(const Path& file_path) {
		close();

		file_path_ = file_path;
		auto file_name = toAvFileName(file_path);
		XAMP_LOG_D(logger_, "open AvLib file stream start: {}.", file_name);

		if (use_custom_io_context_) {
			openWithCustomIO(file_path);
		}
		else {
			openWithNativeIO(file_name);
		}
		XAMP_LOG_D(logger_,
			"AvLib input opened: {} format:{} streams:{}.",
			file_name,
			format_context_->iformat != nullptr ? format_context_->iformat->name : "unknown",
			format_context_->nb_streams);

		AvIfFailedThrow(LIB_AV_LIB.Format->avformat_find_stream_info(format_context_, nullptr));
		XAMP_LOG_D(logger_,
			"AvLib stream info loaded: {} duration:{:.2f}s bit_rate:{}kbps.",
			file_name,
			format_context_->duration > 0
				? static_cast<double>(format_context_->duration) / static_cast<double>(AV_TIME_BASE)
				: 0.0,
			format_context_->bit_rate > 0
				? static_cast<uint32_t>(format_context_->bit_rate / 1000)
				: 0);

		const auto stream_index = LIB_AV_LIB.Format->av_find_best_stream(
			format_context_,
			AVMEDIA_TYPE_AUDIO,
			-1,
			-1,
			nullptr,
			0);
		if (stream_index < 0) {
			throwException<NotSupportFormatException>("No audio stream found in {}.", file_name);
		}
		audio_stream_index_ = stream_index;
		audio_stream_ = format_context_->streams[audio_stream_index_];
		auto* codec_parameters = audio_stream_->codecpar;
		auto* decoder = LIB_AV_LIB.Codec->avcodec_find_decoder(codec_parameters->codec_id);
		XAMP_LOG_D(logger_,
			"AvLib selected audio stream:{} codec_id:{} sample_rate:{} channels:{} sample_fmt:{}.",
			audio_stream_index_,
			static_cast<int>(codec_parameters->codec_id),
			codec_parameters->sample_rate,
			codec_parameters->ch_layout.nb_channels,
			codec_parameters->format >= 0
				? LIB_AV_LIB.Util->av_get_sample_fmt_name(static_cast<AVSampleFormat>(codec_parameters->format))
				: "unknown");
		if (decoder == nullptr) {
			throwException<NotSupportFormatException>(
				"No FFmpeg decoder found. codec id:{} file:{}.",
				static_cast<int>(codec_parameters->codec_id),
				file_name);
		}
		XAMP_LOG_D(logger_, "AvLib decoder selected: {}.", decoder->name != nullptr ? decoder->name : "unknown");

		codec_context_.reset(LIB_AV_LIB.Codec->avcodec_alloc_context3(decoder));
		if (!codec_context_) {
			throw std::bad_alloc();
		}
		AvIfFailedThrow(LIB_AV_LIB.Codec->avcodec_parameters_to_context(
			codec_context_.get(),
			codec_parameters));
		AvIfFailedThrow(LIB_AV_LIB.Codec->avcodec_open2(
			codec_context_.get(),
			decoder,
			nullptr));

		packet_.reset(LIB_AV_LIB.Codec->av_packet_alloc());
		frame_.reset(LIB_AV_LIB.Util->av_frame_alloc());
		if (!packet_ || !frame_) {
			throw std::bad_alloc();
		}

		output_channels_ = getChannelCount(codec_context_.get());
		if (output_channels_ <= 0) {
			throwException<NotSupportFormatException>("Invalid audio channel count. file:{}.", file_name);
		}
		output_sample_rate_ = codec_context_->sample_rate;
		if (output_sample_rate_ <= 0) {
			throwException<NotSupportFormatException>("Invalid audio sample rate. file:{}.", file_name);
		}

		format_ = AudioFormat(DataFormat::FORMAT_PCM,
			static_cast<uint16_t>(output_channels_),
			ByteFormat::FLOAT32,
			static_cast<uint32_t>(output_sample_rate_));
		bit_depth_ = resolveBitDepth(codec_parameters);
		bit_rate_ = codec_parameters->bit_rate > 0
			? static_cast<uint32_t>(codec_parameters->bit_rate / 1000)
			: 0;
		duration_ = resolveDuration();
        if (integer_pcm_) {
            const auto codec = codec_parameters->codec_id;
            const bool integer_codec = codec == AV_CODEC_ID_FLAC || codec == AV_CODEC_ID_PCM_S16LE ||
                codec == AV_CODEC_ID_PCM_S16BE || codec == AV_CODEC_ID_PCM_S24LE ||
                codec == AV_CODEC_ID_PCM_S24BE || codec == AV_CODEC_ID_PCM_S32LE || codec == AV_CODEC_ID_PCM_S32BE;
            if (codec == AV_CODEC_ID_FLAC) {
                const int original_bits = codec_parameters->bits_per_raw_sample > 0
                    ? codec_parameters->bits_per_raw_sample : codec_context_->bits_per_raw_sample;
                if (original_bits <= 0) throw std::runtime_error("Unknown FLAC integer precision");
                bit_depth_ = static_cast<uint32_t>(original_bits);
            }
            const auto sf = codec_context_->sample_fmt;
            if (!integer_codec || output_channels_ != 2 ||
                (bit_depth_ != 16 && bit_depth_ != 24 && bit_depth_ != 32) ||
                (sf != AV_SAMPLE_FMT_S16 && sf != AV_SAMPLE_FMT_S16P &&
                 sf != AV_SAMPLE_FMT_S32 && sf != AV_SAMPLE_FMT_S32P))
                throw std::runtime_error("BitPerfect requires stereo 16/24/32-bit integer PCM WAV or FLAC");
            format_.setByteFormat(ByteFormat::SINT32);
        } else {
            initializeResampler();
        }

		active_ = true;
		eof_ = false;

		XAMP_LOG_D(logger_,
			"open AvLib file stream: {} format:{} duration:{:.2f}s bit_depth:{} bitrate:{}kbps.",
			file_name,
			format_,
			duration_,
			bit_depth_,
			bit_rate_);
	}

	void open(ArchiveEntry) {
		throwException<NotSupportFormatException>("AvLibFileStream does not support archive entry yet.");
	}

	void close() {
        seek_frame_.reset();
		if (format_context_ != nullptr) {
			XAMP_LOG_D(logger_, "close AvLib file stream: {}.", toAvFileName(file_path_));
		}
		pending_samples_.clear();
		pending_sample_offset_ = 0;
        integer_pending_.clear();
        integer_offset_ = 0;
		swr_context_.reset();
		frame_.reset();
		packet_.reset();
		codec_context_.reset();
		if (format_context_ != nullptr) {
			LIB_AV_LIB.Format->avformat_close_input(&format_context_);
			format_context_ = nullptr;
		}
		input_io_context_.reset();
		custom_io_context_.reset();
		audio_stream_ = nullptr;
		audio_stream_index_ = -1;
		output_channels_ = 0;
		output_sample_rate_ = 0;
		bit_depth_ = 0;
		bit_rate_ = 0;
		duration_ = 0.0;
		format_.reset();
		active_ = false;
		eof_ = true;
	}

	[[nodiscard]] double getDuration() const {
		return duration_;
	}

    void setIntegerPcm(bool enabled) {
        if (format_context_) throw std::logic_error("Set PCM mode before opening the stream");
        integer_pcm_ = enabled;
    }
    std::optional<xamp::pcm::Format> integerPcmFormat() const {
        if (!integer_pcm_ || !codec_context_) return std::nullopt;
        return xamp::pcm::canonical(bit_depth_, output_channels_, output_sample_rate_);
    }

	[[nodiscard]] AudioFormat getFormat() const {
		return format_;
	}

	[[nodiscard]] bool endOfStream() const {
		return eof_;
	}

	void seek(double stream_time) {
		if (!format_context_ || !codec_context_ || audio_stream_index_ < 0 || audio_stream_ == nullptr) {
			XAMP_LOG_D(logger_, "AvLib seek ignored because stream is not opened. target:{:.3f}s.", stream_time);
			return;
		}

		auto target = static_cast<int64_t>(
			stream_time * static_cast<double>(audio_stream_->time_base.den)
			/ static_cast<double>(audio_stream_->time_base.num));

        if (integer_pcm_) {
            seek_frame_ = static_cast<int64_t>(std::llround(stream_time * output_sample_rate_));
            if (audio_stream_->start_time != AV_NOPTS_VALUE) target += audio_stream_->start_time;
        }
		auto flags = AVSEEK_FLAG_BACKWARD;
		AvIfFailedThrow(LIB_AV_LIB.Format->av_seek_frame(
			format_context_,
			audio_stream_index_,
			target,
			flags));
		LIB_AV_LIB.Codec->avcodec_flush_buffers(codec_context_.get());
		pending_samples_.clear();
		pending_sample_offset_ = 0;
        integer_pending_.clear();
        integer_offset_ = 0;
		eof_ = false;
		active_ = true;
		if (swr_context_) {
			LIB_AV_LIB.Swr->swr_close(swr_context_.get());
			AvIfFailedThrow(LIB_AV_LIB.Swr->swr_init(swr_context_.get()));
		}
		XAMP_LOG_D(logger_,
			"AvLib seek completed: target:{:.3f}s timestamp:{} stream:{}.",
			stream_time,
			target,
			audio_stream_index_);
	}

	[[nodiscard]] uint32_t getSamples(void* buffer, uint32_t length) {
		if (buffer == nullptr || length == 0 || !active_) {
			return 0;
		}

        if (integer_pcm_) return readInteger(static_cast<std::byte*>(buffer), length);
		auto* output = static_cast<float*>(buffer);
		uint32_t copied_samples = 0;

		while (copied_samples < length) {
			const auto copied_from_pending = copyPendingSamples(
				output + copied_samples,
				length - copied_samples);
			copied_samples += copied_from_pending;
			if (copied_samples >= length) {
				break;
			}

			if (eof_) {
				active_ = hasPendingSamples();
				break;
			}

			if (!decodeNextFrame()) {
				eof_ = true;
				XAMP_LOG_D(logger_, "AvLib reached input EOF, draining decoder.");
				drainDecoder();
				if (!hasPendingSamples()) {
					active_ = false;
					XAMP_LOG_D(logger_, "AvLib stream drained.");
				}
			}
		}

		return copied_samples;
	}

	[[nodiscard]] uint32_t getSampleSize() const {
		return integer_pcm_ ? sizeof(int32_t) : sizeof(float);
	}

	[[nodiscard]] bool isActive() const {
		return active_ || hasPendingSamples();
	}

	[[nodiscard]] uint32_t getBitDepth() const {
		return bit_depth_;
	}

	[[nodiscard]] uint32_t getBitRate() const {
		return bit_rate_;
	}

private:
	void openWithNativeIO(const std::string& file_name) {
		AVFormatContext* raw_format_context = nullptr;
		AvIfFailedThrow(LIB_AV_LIB.Format->avformat_open_input(
			&raw_format_context,
			file_name.c_str(),
			nullptr,
			nullptr));
		format_context_ = raw_format_context;
	}

	void openWithCustomIO(const Path& file_path) {
		custom_io_context_ = makeAlign<AvFastIOContext>(file_path);

		auto* avio_buffer = static_cast<uint8_t*>(LIB_AV_LIB.Util->av_malloc(kAvIOBufferSize));
		if (avio_buffer == nullptr) {
			throw std::bad_alloc();
		}

		auto* raw_io_context = LIB_AV_LIB.Format->avio_alloc_context(
			avio_buffer,
			kAvIOBufferSize,
			0,
			custom_io_context_.get(),
			&AvFastIOContext::readPacket,
			nullptr,
			&AvFastIOContext::seek);
		if (raw_io_context == nullptr) {
			LIB_AV_LIB.Util->av_free(avio_buffer);
			throw std::bad_alloc();
		}
		input_io_context_.reset(raw_io_context);

		format_context_ = LIB_AV_LIB.Format->avformat_alloc_context();
		if (format_context_ == nullptr) {
			throw std::bad_alloc();
		}
		format_context_->pb = input_io_context_.get();
		format_context_->flags |= AVFMT_FLAG_CUSTOM_IO;

		auto* raw_format_context = format_context_;
		AvIfFailedThrow(LIB_AV_LIB.Format->avformat_open_input(
			&raw_format_context,
			nullptr,
			nullptr,
			nullptr));
		format_context_ = raw_format_context;
	}

	void initializeResampler() {
		const auto input_channel_layout = getChannelLayout(codec_context_.get());
		if (input_channel_layout == 0) {
			throwException<NotSupportFormatException>("Unsupported channel layout.");
		}

		swr_context_.reset(LIB_AV_LIB.Swr->swr_alloc_set_opts(
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
		AvIfFailedThrow(LIB_AV_LIB.Swr->swr_init(swr_context_.get()));
		XAMP_LOG_D(logger_,
			"AvLib resampler ready: input:{}Hz/{}ch/{} output:{}Hz/{}ch/{} layout:0x{:X}.",
			codec_context_->sample_rate,
			getChannelCount(codec_context_.get()),
			LIB_AV_LIB.Util->av_get_sample_fmt_name(codec_context_->sample_fmt),
			output_sample_rate_,
			output_channels_,
			LIB_AV_LIB.Util->av_get_sample_fmt_name(kOutputSampleFormat),
			input_channel_layout);
	}

	[[nodiscard]] uint32_t resolveBitDepth(const AVCodecParameters* codec_parameters) const {
		auto bits = codec_parameters->bits_per_raw_sample;
		if (bits <= 0) {
			bits = codec_parameters->bits_per_coded_sample;
		}
		if (bits <= 0) {
			bits = LIB_AV_LIB.Codec->av_get_bits_per_sample(codec_parameters->codec_id);
		}
		return bits > 0 ? static_cast<uint32_t>(bits) : format_.getBitsPerSample();
	}

	[[nodiscard]] double resolveDuration() const {
		if (audio_stream_ != nullptr && audio_stream_->duration > 0) {
			return rationalToSeconds(audio_stream_->duration, audio_stream_->time_base);
		}
		if (format_context_ != nullptr && format_context_->duration > 0) {
			return static_cast<double>(format_context_->duration) / static_cast<double>(AV_TIME_BASE);
		}
		return 0.0;
	}

	[[nodiscard]] bool hasPendingSamples() const {
        return integer_pcm_ ? integer_offset_ < integer_pending_.size()
            : pending_sample_offset_ < pending_samples_.size();
	}

	uint32_t copyPendingSamples(float* output, uint32_t available_samples) {
		if (!hasPendingSamples()) {
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
		if (!hasPendingSamples()) {
			pending_samples_.clear();
			pending_sample_offset_ = 0;
		}
		return static_cast<uint32_t>(copy_count);
	}

	bool decodeNextFrame() {
		while (true) {
			const auto receive_result = LIB_AV_LIB.Codec->avcodec_receive_frame(
				codec_context_.get(),
				frame_.get());
			if (receive_result == 0) {
				convertFrame(frame_.get());
				LIB_AV_LIB.Util->av_frame_unref(frame_.get());
				return true;
			}
			if (receive_result != AVERROR(EAGAIN)) {
				if (receive_result == AVERROR_EOF) {
					return false;
				}
				AvIfFailedThrow(receive_result);
			}

			while (true) {
				const auto read_result = LIB_AV_LIB.Format->av_read_frame(format_context_, packet_.get());
				if (read_result < 0) {
					XAMP_LOG_D(logger_, "AvLib av_read_frame reached EOF/error:{}.", read_result);
					const auto send_result = LIB_AV_LIB.Codec->avcodec_send_packet(codec_context_.get(), nullptr);
					if (send_result != 0 && send_result != AVERROR_EOF) {
						AvIfFailedThrow(send_result);
					}
					return false;
				}

				if (packet_->stream_index == audio_stream_index_) {
					const auto send_result = LIB_AV_LIB.Codec->avcodec_send_packet(
						codec_context_.get(),
						packet_.get());
					LIB_AV_LIB.Codec->av_packet_unref(packet_.get());
					if (send_result == AVERROR(EAGAIN)) {
						break;
					}
					AvIfFailedThrow(send_result);
					break;
				}
				LIB_AV_LIB.Codec->av_packet_unref(packet_.get());
			}
		}
	}

	void drainDecoder() {
		while (true) {
			const auto receive_result = LIB_AV_LIB.Codec->avcodec_receive_frame(
				codec_context_.get(),
				frame_.get());
			if (receive_result == AVERROR_EOF || receive_result == AVERROR(EAGAIN)) {
				return;
			}
			AvIfFailedThrow(receive_result);
			convertFrame(frame_.get());
			LIB_AV_LIB.Util->av_frame_unref(frame_.get());
		}
	}

    uint32_t readInteger(std::byte* output, uint32_t length) {
        size_t copied = 0;
        const size_t capacity = static_cast<size_t>(length) * 4;
        while (copied < capacity) {
            const auto count = (std::min)(capacity-copied, integer_pending_.size()-integer_offset_);
            if (count) {
                std::memcpy(output+copied, integer_pending_.data()+integer_offset_, count);
                copied += count; integer_offset_ += count;
            }
            if (copied == capacity) break;
            integer_pending_.clear(); integer_offset_ = 0;
            if (eof_) { active_ = false; break; }
            if (!decodeNextFrame()) {
                eof_ = true;
                drainDecoder();
            }
        }
        return static_cast<uint32_t>(copied/4);
    }
    void appendInteger(AVFrame* frame) {
        const bool planar = frame->format == AV_SAMPLE_FMT_S16P || frame->format == AV_SAMPLE_FMT_S32P;
        const bool short_samples = frame->format == AV_SAMPLE_FMT_S16 || frame->format == AV_SAMPLE_FMT_S16P;
        if (frame->format != AV_SAMPLE_FMT_S16 && frame->format != AV_SAMPLE_FMT_S16P &&
            frame->format != AV_SAMPLE_FMT_S32 && frame->format != AV_SAMPLE_FMT_S32P)
            throw std::runtime_error("BitPerfect decoder changed to non-integer samples");
        if (frame->sample_rate != output_sample_rate_ || frame->ch_layout.nb_channels != output_channels_)
            throw std::runtime_error("BitPerfect decoder changed stream format");
        if (short_samples && bit_depth_ > 16) throw std::runtime_error("Decoder integer precision too low");
        size_t skip = 0;
        if (seek_frame_) {
            const auto pts = frame->best_effort_timestamp;
            if (pts == AV_NOPTS_VALUE) {
                if (*seek_frame_ != 0) throw std::runtime_error("Exact PCM seek requires timestamps");
            } else {
                const auto origin = audio_stream_->start_time == AV_NOPTS_VALUE ? 0 : audio_stream_->start_time;
                const auto start = static_cast<int64_t>(std::llround(
                    static_cast<long double>(pts-origin) * audio_stream_->time_base.num * output_sample_rate_
                    / audio_stream_->time_base.den));
                if (*seek_frame_ < start) throw std::runtime_error("Decoder seek skipped required samples");
                if (*seek_frame_ >= start+frame->nb_samples) return;
                skip = static_cast<size_t>(*seek_frame_-start);
            }
            seek_frame_.reset();
        }
        const auto old = integer_pending_.size();
        integer_pending_.resize(old+(frame->nb_samples-skip)*output_channels_*4);
        auto* out = integer_pending_.data()+old;
        const size_t width = short_samples ? 2 : 4;
        for (size_t f=skip; f<static_cast<size_t>(frame->nb_samples); ++f) {
            for (int ch=0; ch<output_channels_; ++ch) {
                const auto* in = frame->extended_data[planar ? ch : 0]+(planar ? f : f*output_channels_+ch)*width;
                uint32_t word;
                if (short_samples) {
                    int16_t sample; std::memcpy(&sample,in,2);
                    word = static_cast<uint32_t>(static_cast<int32_t>(sample)) << 16;
                } else {
                    std::memcpy(&word,in,4);
                }
                if (bit_depth_ < 32 && (word & (UINT32_MAX >> bit_depth_)))
                    throw std::runtime_error("Integer decoder returned unexpected padding bits");
                for (unsigned b=0; b<4; ++b) *out++ = static_cast<std::byte>((word>>(8*b))&255);
            }
        }
    }

	void convertFrame(AVFrame* frame) {
		if (frame == nullptr || frame->nb_samples <= 0) {
			return;
		}

        if (integer_pcm_) { appendInteger(frame); return; }
		const auto max_output_samples = LIB_AV_LIB.Swr->swr_get_out_samples(
			swr_context_.get(),
			frame->nb_samples);
		if (max_output_samples <= 0) {
			return;
		}

		output_samples_.resize(static_cast<size_t>(max_output_samples) * static_cast<size_t>(output_channels_));
		auto* output_data = reinterpret_cast<uint8_t*>(output_samples_.data());
		auto converted_samples = LIB_AV_LIB.Swr->swr_convert(
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
	
    bool integer_pcm_{false};
    std::vector<std::byte> integer_pending_;
    size_t integer_offset_{0};
    std::optional<int64_t> seek_frame_;
	Path file_path_;
	Buffer<float> output_samples_;
	std::vector<float> pending_samples_;
	LoggerPtr logger_;
	ScopedPtr<AvFastIOContext> custom_io_context_;
	AvPtr<AVCodecContext> codec_context_;
	AvPtr<AVIOContext> input_io_context_;
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
	bool use_custom_io_context_{ false };
	bool active_{ false };
	bool eof_{ true };
};

AvLibFileStream::AvLibFileStream()
	: impl_(makeAlign<AvLibFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(AvLibFileStream)

void AvLibFileStream::setIntegerPcm(bool enabled) { impl_->setIntegerPcm(enabled); }
std::optional<xamp::pcm::Format> AvLibFileStream::integerPcmFormat() const { return impl_->integerPcmFormat(); }

void AvLibFileStream::openFile(const Path& file_path) {
	impl_->openFile(file_path);
}

void AvLibFileStream::useCustomIOContext(bool enable) {
	impl_->useCustomIOContext(enable);
}

void AvLibFileStream::open(ArchiveEntry archive_entry) {
	impl_->open(std::move(archive_entry));
}

void AvLibFileStream::close() {
	impl_->close();
}

double AvLibFileStream::getDuration() const {
	return impl_->getDuration();
}

AudioFormat AvLibFileStream::getFormat() const {
	return impl_->getFormat();
}

void AvLibFileStream::seek(double stream_time) const {
	impl_->seek(stream_time);
}

uint32_t AvLibFileStream::getSamples(void* buffer, uint32_t length) const {
	return impl_->getSamples(buffer, length);
}

uint32_t AvLibFileStream::getSampleSize() const {
	return impl_->getSampleSize();
}

bool AvLibFileStream::isActive() const {
	return impl_->isActive();
}

uint32_t AvLibFileStream::getBitDepth() const {
	return impl_->getBitDepth();
}

uint32_t AvLibFileStream::getBitRate() const {
	return impl_->getBitRate();
}

bool AvLibFileStream::endOfStream() const {
	return impl_->endOfStream();
}


XAMP_STREAM_NAMESPACE_END
