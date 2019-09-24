#include <stream/avfilestream.h>

#define ENABLE_IO_CONTEXT 0

#pragma comment(lib, "crypt32")
#pragma comment(lib, "Bcrypt")
#pragma comment(lib, "ws2_32")

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/file.h>
#include <libswresample/swresample.h>
#include <libavutil/rational.h>
#include <libavutil/opt.h>
}

#include <cassert>
#include <base/exception.h>
#include <base/file.h>
#include <base/memory.h>
#include <base/exception.h>
#include <base/unicode.h>
#include <base/defer.h>

namespace xamp::stream {

class AvException : public Exception {
public:
    explicit AvException(int32_t error)
        : Exception(Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR) {
        char buf[256]{};
        av_strerror(error, buf, sizeof(buf) - 1);
        what_.assign(buf);
    }
};

#define AV_IF_FAILED_THROW(expr) \
	do { \
		auto error = (expr); \
		if (error != 0) { \
			throw AvException(error); \
		} \
	} while (false)

template <typename T>
struct AvResourceDeleter;

template <>
struct AvResourceDeleter<AVFormatContext> {
	void operator()(AVFormatContext* p) const {
		assert(p != nullptr);
#ifndef ENABLE_IO_CONTEXT
		if (!(p->flags & AVFMT_NOFILE)) {
			avio_close(p->pb);
		}
		avformat_free_context(p);
#endif
		avformat_close_input(&p);
	}
};

template <>
struct AvResourceDeleter<AVCodecContext> {
	void operator()(AVCodecContext* p) const {
		assert(p != nullptr);
		avcodec_close(p);
	}
};

template <>
struct AvResourceDeleter<SwrContext> {
	void operator()(SwrContext* p) const {
		assert(p != nullptr);
		swr_free(&p);
	}
};

template <>
struct AvResourceDeleter<AVPacket> {
	void operator()(AVPacket* p) const {
		assert(p != nullptr);
		av_packet_unref(p);
		av_packet_free(&p);
	}
};

template <>
struct AvResourceDeleter<AVFrame> {
	void operator()(AVFrame* p) const {
		assert(p != nullptr);
		av_free(p);
	}
};

template <typename T>
using AvPtr = std::unique_ptr<T, AvResourceDeleter<T>>;

class LibAvInit {
public:
	XAMP_DISABLE_COPY(LibAvInit)

	static LibAvInit& Instance() {
		static LibAvInit instance;
		return instance;
	}

protected:
	LibAvInit() {
		av_log_set_level(AV_LOG_FATAL);
		av_log_set_callback(AvLogCallback);
	}	

private:
	static void AvLogCallback(void* ptr, int level, const char* fmt, va_list args) {
		if (level != log_level) {
			return;
		}		
		static const int32_t MSG_BUF_MAX_LEN = 1024;
		char msgbuf[MSG_BUF_MAX_LEN]{};
		vsnprintf(msgbuf, sizeof(msgbuf) - 1, fmt, args);
		auto s = strstr(msgbuf, "\n");
		if (s != nullptr) {
			s[0] = '\0';
		}
	}

	static int log_level;
};

int LibAvInit::log_level = AV_LOG_FATAL;

struct AvStreamContext {
	AvStreamContext()
		: buffer(nullptr)
		, offset(0)
		, size(0) {
	}

	static int64_t OnSeekPacketCallback(void* opaque, int64_t offset, int whence) {
		auto context = reinterpret_cast<AvStreamContext*>(opaque);

		switch (whence) {
		case SEEK_CUR:
			context->offset += offset;
			break;
		case SEEK_END:
			context->offset = context->size + offset;
			break;
		case SEEK_SET:
			context->offset = offset;
			break;
		case AVSEEK_SIZE:
			return context->size;
		default:
			break;
		}
		return context->offset;
	}

	static int OnReadPacketCallback(void* opaque, uint8_t* buf, int buf_size) {
		auto context = reinterpret_cast<AvStreamContext*>(opaque);

		const auto reminder = context->size - context->offset;
		buf_size = FFMIN(buf_size, reminder);

		if (buf_size < 0) {
			return AVERROR_EOF;
		}

		FastMemcpy(buf, context->buffer + context->offset, buf_size);
		context->offset += buf_size;
		return buf_size;
	}

	uint8_t* buffer;
	int64_t offset;
	size_t size;
};

class AvFileStream::AvFileStreamImpl {
public:
	AvFileStreamImpl()
		: open_mode_(OpenMode::NOT_IN_MEMORY)
		, video_stream_id_(-1)
		, audio_stream_id_(-1)
		, duration_(0) {
		LibAvInit::Instance();
	}

	~AvFileStreamImpl() {
		Close();
	}

	bool HasAudio() const noexcept {
		return audio_stream_id_ >= 0;
	}

	void Close() {
		video_stream_id_ = -1;
		audio_stream_id_ = -1;
		duration_ = 0;
		audio_format_ = AudioFormat::UnknowFormat;
#if ENABLE_IO_CONTEXT
		if (iocontext_.buffer != nullptr) {
			av_freep(&avio_context_->buffer);
			av_freep(&avio_context_);
			av_file_unmap(iocontext_.buffer, iocontext_.size);
			avio_context_ = nullptr;
		}
		iocontext_.buffer = nullptr;
		iocontext_.size = 0;
#endif
		swr_context_.reset();
		audio_context_.reset();
		video_context_.reset();
		audio_contex_.reset();
		format_context_.reset();
	}

	void LoadFromFile(const std::wstring& file_path) {
		AVFormatContext* format_ctx = nullptr;
#if ENABLE_IO_CONTEXT
		uint8_t* buffer = nullptr;
		size_t buffer_size;

		auto file_path_ut8 = ToUtf8String(file_path);
		if (av_file_map(file_path_ut8.c_str(), &buffer, &buffer_size, 0, nullptr) < 0) {
			throw FileNotFoundException();
		}

		format_ctx = avformat_alloc_context();
		const auto avio_ctx_buffer_size = 4096;
		const auto avio_ctx_buffer = reinterpret_cast<uint8_t*>(av_malloc(avio_ctx_buffer_size));

		iocontext_.buffer = buffer;
		iocontext_.offset = 0;
		iocontext_.size = buffer_size;

		PrefetchMemory(iocontext_.buffer, iocontext_.size);

		avio_context_ = avio_alloc_context(avio_ctx_buffer,
			avio_ctx_buffer_size,
			0,
			&iocontext_,
			&AvStreamContext::OnReadPacketCallback,
			nullptr,
			&AvStreamContext::OnSeekPacketCallback);

		format_ctx->pb = avio_context_;

		if (avformat_open_input(&format_ctx, nullptr, nullptr, nullptr) != 0) {
			throw FileNotFoundException();
		}
#else		
		auto file_path_ut8 = ToUtf8String(file_path);		
		if (avformat_open_input(&format_ctx, file_path_ut8.c_str(), nullptr, nullptr) != 0) {
			throw FileNotFoundException();
		}
#endif
		if (format_ctx != nullptr) {
			format_context_.reset(format_ctx);
		} else {
			throw FileNotFoundException();
		}

		if (avformat_find_stream_info(format_context_.get(), nullptr) < 0) {
			throw FileNotFoundException();
		}

		for (auto i = 0; i < format_context_->nb_streams; ++i) {
			if ((video_stream_id_ < 0) && (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)) {
				video_stream_id_ = i;
			}

			if ((audio_stream_id_ < 0) && (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)) {
				audio_stream_id_ = i;
			}
		}

		if (HasAudio()) {
			OpenAudioStream();
		} else {
			throw FileNotFoundException();
		}

		const auto stream = format_context_->streams[audio_stream_id_];
		duration_ = av_q2d(stream->time_base) * static_cast<double>(stream->duration);
	}

	void OpenAudioStream() {
		audio_contex_.reset(format_context_->streams[audio_stream_id_]->codec);

		const auto codec = avcodec_find_decoder(audio_contex_->codec_id);
		if (!codec) {
			throw FileNotFoundException();
		}

		AV_IF_FAILED_THROW(avcodec_open2(audio_contex_.get(), codec, nullptr));
		audio_context_.reset(av_frame_alloc());

		auto interleaved_format = InterleavedFormat::INTERLEAVED;
		const auto output_format = AV_SAMPLE_FMT_FLT;

		switch (audio_contex_->sample_fmt) {
		case AV_SAMPLE_FMT_U8P:
		case AV_SAMPLE_FMT_S32P:
			throw NotSupportFormatException();
			break;
		case AV_SAMPLE_FMT_S16P:		
		case AV_SAMPLE_FMT_FLTP:
			interleaved_format = InterleavedFormat::DEINTERLEAVED;
			break;
		default:
			break;
		}

		swr_context_.reset(swr_alloc_set_opts(swr_context_.get(),
			AV_CH_LAYOUT_STEREO,
			output_format,
			audio_contex_->sample_rate,
			audio_contex_->channel_layout == 0 ? AV_CH_LAYOUT_STEREO : audio_contex_->channel_layout,
			audio_contex_->sample_fmt,
			audio_contex_->sample_rate,
			0,
			nullptr));

		AV_IF_FAILED_THROW(swr_init(swr_context_.get()));

		audio_format_ = AudioFormat(audio_contex_->channels,
			av_get_bytes_per_sample(audio_contex_->sample_fmt) * 8,
			audio_contex_->sample_rate);
		audio_format_.SetInterleavedFormat(interleaved_format);
	}

	int32_t GetSamples(float* buffer, const int32_t length) noexcept {
		auto num_read_sample = 0;
		for (uint32_t i = 0; i < format_context_->nb_streams; ++i) {
			AvPtr<AVPacket> packet(av_packet_alloc());
			av_init_packet(packet.get());
			while (av_read_frame(format_context_.get(), packet.get()) >= 0) {
				XAMP_DEFER(
					av_packet_unref(packet.get());
				);
				if (packet->stream_index != audio_stream_id_) {
					continue;
				}
				auto ret = avcodec_send_packet(audio_contex_.get(), packet.get());
				if (ret == AVERROR(EAGAIN)) {
					break;
				}
				if (ret == AVERROR_EOF || ret == AVERROR(EINVAL)) {
					return num_read_sample;
				}
				if (ret == AVERROR_INVALIDDATA) {
					break;
				}
				while (ret >= 0) {
					XAMP_DEFER(
						av_frame_unref(audio_context_.get());
					);
					ret = avcodec_receive_frame(audio_contex_.get(), audio_context_.get());
					if (ret == AVERROR(EAGAIN)) {
						break;
					}
					if (ret == AVERROR_EOF || ret == AVERROR(EINVAL)) {
						return num_read_sample;
					}
					ret = ConvertSamples(buffer + num_read_sample, length - num_read_sample);
					num_read_sample += ret;
					if (num_read_sample + ret > length / 2) {
						return num_read_sample;
					}
				}
			}
		}
		return num_read_sample;
	}

	AudioFormat GetFormat() const noexcept {
		return audio_format_;
	}

	double GetDuration() const noexcept {
		return duration_;
	}

	int32_t GetSampleSize() const noexcept {
		return sizeof(float);
	}

	void Seek(const double stream_time) const {
		auto timestamp = static_cast<int64_t>(stream_time * AV_TIME_BASE);

		if (format_context_->start_time != AV_NOPTS_VALUE) {
			timestamp += format_context_->start_time;
		}

		AV_IF_FAILED_THROW(av_seek_frame(format_context_.get(), -1, timestamp, AVSEEK_FLAG_BACKWARD));
		avcodec_flush_buffers(audio_contex_.get());
	}

private:
	int32_t ConvertSamples(float* buffer, const uint32_t length) const noexcept {
		const auto frame_size = audio_context_->nb_samples * audio_contex_->channels;
		const auto result = swr_convert(swr_context_.get(),
			reinterpret_cast<uint8_t **>(&buffer),
			audio_context_->nb_samples,
			const_cast<const uint8_t **>(audio_context_->extended_data),
			audio_context_->nb_samples);
		assert(result > 0);
		const auto convert_size = result * audio_contex_->channels;
		assert(convert_size == frame_size && convert_size < length);
		return frame_size;
	}

	OpenMode open_mode_;
	int32_t video_stream_id_;
	int32_t audio_stream_id_;
	double duration_;
	std::wstring codec_name_;
#if ENABLE_IO_CONTEXT
	AvStreamContext iocontext_;
	AVIOContext* avio_context_{nullptr};
#endif
	AudioFormat audio_format_;
	AvPtr<SwrContext> swr_context_;
	AvPtr<AVFrame> audio_context_;
	AvPtr<AVCodecContext> video_context_;
	AvPtr<AVCodecContext> audio_contex_;
	AvPtr<AVFormatContext> format_context_;
};

AvFileStream::AvFileStream()
	: impl_(std::make_unique<AvFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(AvFileStream)

void AvFileStream::OpenFromFile(const std::wstring& file_path, OpenMode open_mode) {
	return impl_->LoadFromFile(file_path);
}

void AvFileStream::Close() {
	impl_->Close();
}

double AvFileStream::GetDuration() const {
	return impl_->GetDuration();
}

AudioFormat AvFileStream::GetFormat() const {
	return impl_->GetFormat();
}

int32_t AvFileStream::GetSamples(void* buffer, int32_t length) const noexcept {
	return impl_->GetSamples((float *)buffer, length);
}

void AvFileStream::Seek(double stream_time) const {
	impl_->Seek(stream_time);
}

std::wstring AvFileStream::GetStreamName() const {
	return L"LibAv";
}

int32_t AvFileStream::GetSampleSize() const {
	return impl_->GetSampleSize();
}

bool AvFileStream::IsDSDFile() const {
	return false;
}

}

