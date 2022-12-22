#include <stream/avfileencoder.h>
#include <stream/avfilestream.h>

#include <stream/avlib.h>
#include <stream/dsd_utils.h>

#include <fstream>

namespace xamp::stream {

static void Encode(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt) {

}

static int SelectHighestChannelLayout(const AVCodec* codec) {
    if (!codec->channel_layouts) {
        return AV_CH_LAYOUT_STEREO;
    }
    auto best_nb_channels = 0;
    const auto* channel_layouts = codec->channel_layouts;
    while (*channel_layouts) {
        best_nb_channels = (std::max)(best_nb_channels,
            LIBAV_LIB.UtilLib->av_get_channel_layout_nb_channels(*channel_layouts));
    }
    return best_nb_channels;
}

static int32_t SelectHighestSampleRate(const AVCodec* codec) noexcept {
    if (!codec->supported_samplerates) {
        return kPcmSampleRate441;
    }

    auto best_sample_rate = 0;
    const auto *sample_rate = codec->supported_samplerates;
    while (*sample_rate) {
        best_sample_rate = (std::max)(*sample_rate, best_sample_rate);
    }
    return best_sample_rate;
}

static bool CheckSampleFmt(const AVCodec* codec, enum AVSampleFormat sample_fmt) noexcept {
    const enum AVSampleFormat* format = codec->sample_fmts;
    while (*format != AV_SAMPLE_FMT_NONE) {
        if (*format == sample_fmt)
            return true;
        ++format;
    }
    return false;
}

class AvFileEncoder::AvFileEncoderImpl {
public:
    void Start(const AnyMap& config) {
        intput_file_ = MakeAlign<AvFileStream>();

        const auto input_file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
        const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);

        intput_file_->OpenFile(input_file_path);
        output_file_.open(input_file_path, std::ios::binary);

        codec_ = LIBAV_LIB.CodecLib->avcodec_find_encoder(AV_CODEC_ID_MP2);
        if (!codec_) {
            return;
        }

        codec_context_.reset(LIBAV_LIB.CodecLib->avcodec_alloc_context3(codec_));
        codec_context_->bit_rate = 32000;
        codec_context_->sample_fmt = AV_SAMPLE_FMT_S16;
        if (!CheckSampleFmt(codec_, codec_context_->sample_fmt)) {
            return;
        }

        codec_context_->sample_rate = SelectHighestSampleRate(codec_);
        codec_context_->channel_layout = SelectHighestChannelLayout(codec_);
        codec_context_->channels = LIBAV_LIB.UtilLib->av_get_channel_layout_nb_channels(codec_context_->channel_layout);

        AvIfFailedThrow(LIBAV_LIB.CodecLib->avcodec_open2(codec_context_.get(), codec_, nullptr));

        format_context_.reset(LIBAV_LIB.FormatLib->avformat_alloc_context());
        stream_ = LIBAV_LIB.FormatLib->avformat_new_stream(format_context_.get(), nullptr);

        stream_->time_base.den = intput_file_->GetFormat().GetSampleRate();
        stream_->time_base.num = 1;

        if (format_context_->oformat->flags & AVFMT_GLOBALHEADER) {
            codec_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        AvIfFailedThrow(LIBAV_LIB.CodecLib->avcodec_parameters_from_context(stream_->codecpar, codec_context_.get()));
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
        
    }

private:
    std::ofstream output_file_;
    AVCodec* codec_{ nullptr };
    AVStream* stream_{nullptr};
    AlignPtr<AvFileStream> intput_file_;
    AvPtr<AVFormatContext> format_context_;
    AvPtr<AVCodecContext> codec_context_;
};

AvFileEncoder::AvFileEncoder()
	: impl_(MakeAlign<AvFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(AvFileEncoder)

void AvFileEncoder::Start(const AnyMap& config) {
    impl_->Start(config);
}

void AvFileEncoder::Encode(std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(progress);
}

}
