#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/buffer.h>
#include <base/simd.h>
#include <base/dataconverter.h>

#include <string.h>

#include <stream/avlib.h>
#include <stream/api.h>
#include <stream/filestream.h>
#include <stream/alacencoder.h>

XAMP_STREAM_NAMESPACE_BEGIN

namespace {
	enum {
		ALAC_EXTRADATA_SIZE = 36
	};

	/*
	    STSD atom

	    32 bits  atom size
	    32 bits  tag("alac")
	    32 bits  tag version(0)
	    32 bits  samples per frame(used when not set explicitly in the frames)
	    8 bits  compatible version(0)
	    8 bits  sample size
	    8 bits  history mult(40)
	    8 bits  initial history(10)
	    8 bits  rice param limit(14)
	    8 bits  channels
	    16 bits  maxRun(255)
	    32 bits  max coded frame size(0 means unknown)
	    32 bits  average bitrate(0 means unknown)
	    32 bits  samplerate
	    */
    struct StsdAtom {
        uint32_t samples_per_frame;
        uint8_t  compatible_version;
        uint8_t  sample_size;
        uint8_t  history_mult;
        uint8_t  initial_history;
        uint8_t  rice_param_limit;
        uint8_t  channels;
        uint16_t max_run;
        uint32_t max_coded_frame_size;
        uint32_t average_bit_rate;
        uint32_t sample_rate;
    };

}

class AlacFileEncoder::AlacFileEncoderImpl {
public:
    ~AlacFileEncoderImpl() {
        format_context_.reset();
        stream_ = nullptr;
        codec_context_ = nullptr;
    }

    void Start(const AnyMap& config) {
        const auto input_file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
        const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);
        file_name_ = output_file_path.string();
        input_file_ = StreamFactory::MakeFileStream(DsdModes::DSD_MODE_PCM, input_file_path);
        input_file_->OpenFile(input_file_path);
        CreateAudioSteam();
    }

    void CreateAudioSteam() {
        const auto best_sample_format = AV_SAMPLE_FMT_S32P;
        const auto frame_size = 4096;
        const char file_name[] = "output.m4a";

        AVIOContext* output_io_context = nullptr;
        AvIfFailedThrow(LIBAV_LIB.Format->avio_open(&output_io_context, file_name, AVIO_FLAG_WRITE));

        format_context_.reset(LIBAV_LIB.Format->avformat_alloc_context());
        if (!format_context_) {
            throw Exception();
        }

        format_context_->pb = output_io_context;
        strcpy_s(format_context_->filename, file_name);

        auto * av_codec = LIBAV_LIB.Codec->avcodec_find_encoder(AV_CODEC_ID_ALAC);
        if (!av_codec) {
            throw Exception();
        }

        stream_ = LIBAV_LIB.Format->avformat_new_stream(format_context_.get(), av_codec);
        if (!stream_) {
            throw Exception();
        }

        const auto format = input_file_->GetFormat();

        codec_context_ = LIBAV_LIB.Codec->avcodec_alloc_context3(av_codec);
        codec_context_->channels       = 2;
        codec_context_->channel_layout = 3;
        codec_context_->sample_rate    = format.GetSampleRate();
        codec_context_->sample_fmt     = best_sample_format;
        codec_context_->frame_size     = frame_size;

        stream_->id                       = format_context_->nb_streams - 1;
        stream_->codecpar->codec_type     = AVMEDIA_TYPE_AUDIO;
        stream_->codecpar->codec_id       = AV_CODEC_ID_ALAC;
        stream_->codecpar->channel_layout = 3;
        stream_->codecpar->channels       = format.GetChannels();
        stream_->codecpar->sample_rate    = format.GetSampleRate();
        stream_->codecpar->frame_size     = frame_size;
        stream_->codecpar->format         = best_sample_format;
        stream_->time_base = AVRational{ 1, static_cast<int32_t>(format.GetSampleRate()) };
        codec_context_->time_base = stream_->time_base;

        AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_parameters_from_context(stream_->codecpar, codec_context_));

        format_context_->oformat = LIBAV_LIB.Format->av_guess_format(nullptr, file_name, nullptr);
        if (format_context_->oformat->flags & AVFMT_GLOBALHEADER)
            codec_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        codec_context_->extradata = static_cast<uint8_t*>(LIBAV_LIB.Util->av_malloc(ALAC_EXTRADATA_SIZE));
        codec_context_->extradata_size = ALAC_EXTRADATA_SIZE;
        MemorySet(codec_context_->extradata, 0, ALAC_EXTRADATA_SIZE);
        
        // Skip atom size:4, alac tag:4, tag version:4 = 12
        auto buffer = codec_context_->extradata + 12;

        auto atom = reinterpret_cast<StsdAtom*>(buffer);
        auto atom_size = sizeof(StsdAtom);
        atom->samples_per_frame = 1024;
        atom->history_mult      = 40;
        atom->initial_history   = 10;
        atom->rice_param_limit  = 10;
        atom->channels          = 2;
        atom->max_run           = 255;
        atom->sample_rate       = format.GetSampleRate();
        atom->sample_size       = 32;

        stream_->codecpar->extradata = static_cast<uint8_t*>(LIBAV_LIB.Util->av_malloc(codec_context_->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE));
        stream_->codecpar->extradata_size = codec_context_->extradata_size;
        MemorySet(stream_->codecpar->extradata, 0, stream_->codecpar->extradata_size);
        MemoryCopy(stream_->codecpar->extradata, codec_context_->extradata, codec_context_->extradata_size);

        AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_open2(codec_context_, av_codec, nullptr));
        
        AvIfFailedThrow(LIBAV_LIB.Format->avio_open(&format_context_->pb, file_name, AVIO_FLAG_WRITE));
        AvIfFailedThrow(LIBAV_LIB.Format->avformat_write_header(format_context_.get(), nullptr));
    }

    bool EncodeFrame(const AVFrame* frame) const {
    	LIBAV_LIB.Codec->av_init_packet(packet_.get());

        auto ret = LIBAV_LIB.Codec->avcodec_send_frame(codec_context_, frame);
        AvIfFailedThrow(ret);

        while (true) {
            ret = LIBAV_LIB.Codec->avcodec_receive_packet(codec_context_, packet_.get());
            if (ret == AVERROR_EOF) {
                return false;
            }
            if (ret == AVERROR(EAGAIN)) {
                return true;
            }
            else {
                AvIfFailedThrow(ret);
            }
            ret = LIBAV_LIB.Format->av_write_frame(format_context_.get(), packet_.get());
            AvIfFailedThrow(ret);
            LIBAV_LIB.Codec->av_packet_unref(packet_.get());
        }        
    }

    void Encode(const std::function<bool(uint32_t)>& progress) {
        packet_.reset(LIBAV_LIB.Codec->av_packet_alloc());
        
        frame_.reset(LIBAV_LIB.Util->av_frame_alloc());
        frame_->nb_samples     = codec_context_->frame_size;
        frame_->format         = codec_context_->sample_fmt;
        frame_->channel_layout = codec_context_->channel_layout;

        buffer_.resize(codec_context_->frame_size * 2);

        while (true) {
            frame_->pts = pts_;
            pts_ += frame_->nb_samples;

            auto ret = LIBAV_LIB.Util->av_frame_get_buffer(frame_.get(), 0);
            if (ret < 0) {
                AvIfFailedThrow(ret);
            }
            ret = LIBAV_LIB.Util->av_frame_make_writable(frame_.get());
            if (ret < 0) {
                AvIfFailedThrow(ret);
            }

            const auto read_samples = input_file_->GetSamples(buffer_.data(),
                                                              codec_context_->frame_size);
            if (!read_samples) {
                break;
            }

            InterleaveToPlanar<float, int32_t>::Convert(
                buffer_.data(),
                reinterpret_cast<int32_t*>(frame_->data[0]),
                reinterpret_cast<int32_t*>(frame_->data[1]),
                codec_context_->frame_size,
                1.0);

            if (!EncodeFrame(frame_.get())) {
                break;
            }
        }

        while (EncodeFrame(nullptr)) {}

        AvIfFailedThrow(LIBAV_LIB.Format->av_write_trailer(format_context_.get()));
    }
private:
    int64_t pts_{ 0 };
    AVCodecContext* codec_context_{ nullptr };
    AVStream* stream_{ nullptr };
    AlignPtr<FileStream> input_file_;
    AvPtr<AVFormatContext> format_context_;
    AvPtr<AVPacket> packet_;
    AvPtr<AVFrame> frame_;
    std::vector<float> buffer_;
    std::string file_name_;
};

AlacFileEncoder::AlacFileEncoder()
    : impl_(MakeAlign<AlacFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(AlacFileEncoder)

void AlacFileEncoder::Start(const AnyMap& config) {
    impl_->Start(config);
}

void AlacFileEncoder::Encode(std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(progress);
}

XAMP_STREAM_NAMESPACE_END
