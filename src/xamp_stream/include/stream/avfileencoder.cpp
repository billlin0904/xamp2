#include <stream/avfileencoder.h>
#include <stream/avfilestream.h>

#include <fstream>

#include <stream/avlib.h>
#include <stream/dsd_utils.h>

#include <base/scopeguard.h>
#include <base/encodingprofile.h>

namespace xamp::stream {

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

static bool CheckSampleRate(const AVCodec* codec, uint32_t target_sample_rate) noexcept {
    if (!codec->supported_samplerates) {
        return false;
    }

    auto best_sample_rate = 0;
    const auto *sample_rate = codec->supported_samplerates;
    while (*sample_rate) {
        if (*sample_rate == target_sample_rate) {
            return true;
        }
        ++sample_rate;
    }
    return false;
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
	explicit AvFileEncoderImpl(AvEncodeId codec_id)
	    : codec_id_(codec_id) {	    
    }

    void Start(const AnyMap& config) {
        const auto input_file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
        const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);
        const auto encoding_profile = config.Get<EncodingProfile>(FileEncoderConfig::kEncodingProfile);

        input_file_.OpenFile(input_file_path);
        const auto output_file = String::ToUtf8String(output_file_path.wstring());

		AVFormatContext* format_ctx = nullptr;
        AvIfFailedThrow(avformat_alloc_output_context2(&format_ctx, nullptr, nullptr, output_file.c_str()));
        AvIfFailedThrow(avio_open(&format_ctx->pb, output_file.c_str(), AVIO_FLAG_WRITE));

        AVCodecID id = AV_CODEC_ID_NONE;

        switch (codec_id_) {
        case AvEncodeId::AV_ENCODE_ID_AAC:
            id = AVCodecID::AV_CODEC_ID_AAC;
            break;
        case AvEncodeId::AV_ENCODE_ID_WAV:
            id = AVCodecID::AV_CODEC_ID_PCM_U16LE;
            break;
        case AvEncodeId::AV_ENCODE_ID_FLAC:
            id = AVCodecID::AV_CODEC_ID_FLAC;
            break;
        default: 
            break;
        }

        codec_ = LIBAV_LIB.CodecLib->avcodec_find_encoder(id);
        if (!codec_) {
            return;
        }

        codec_context_.reset(LIBAV_LIB.CodecLib->avcodec_alloc_context3(codec_));
        codec_context_->bit_rate = encoding_profile.bitrate * 1000;
        codec_context_->sample_fmt = AV_SAMPLE_FMT_FLTP;
        codec_context_->sample_rate = encoding_profile.sample_rate;
        codec_context_->channel_layout = AV_CH_LAYOUT_STEREO;
        codec_context_->channels = LIBAV_LIB.UtilLib->av_get_channel_layout_nb_channels(codec_context_->channel_layout);

        if (!CheckSampleRate(codec_, codec_context_->sample_rate)) {
            return;
        }
        if (!CheckSampleFmt(codec_, codec_context_->sample_fmt)) {
            return;
        }

        AvIfFailedThrow(LIBAV_LIB.CodecLib->avcodec_open2(codec_context_.get(), codec_, nullptr));

        stream_ = LIBAV_LIB.FormatLib->avformat_new_stream(format_ctx, codec_);
        frame_.reset(LIBAV_LIB.UtilLib->av_frame_alloc());

        frame_->nb_samples = codec_context_->frame_size;
        frame_->format = codec_context_->sample_fmt;
        frame_->channel_layout = codec_context_->channel_layout;

        AvIfFailedThrow(LIBAV_LIB.UtilLib->av_frame_get_buffer(frame_.get(), 0));
        packet_.reset(LIBAV_LIB.CodecLib->av_packet_alloc());
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
	    const auto data_size= LIBAV_LIB.UtilLib->av_samples_get_buffer_size(nullptr,
	        codec_context_->channels,
	        frame_->nb_samples,
            static_cast<AVSampleFormat>(frame_->format),
	        0);

        LIBAV_LIB.FormatLib->avformat_write_header(format_context_.get(), nullptr);

        EncodeToFile(progress);
    }

private:
    void EncodeToFile(std::function<bool(uint32_t) > const& progress) {
        auto ret = LIBAV_LIB.CodecLib->avcodec_send_frame(codec_context_.get(), frame_.get());
        if (ret < 0) {
            return;
        }

        while (ret >= 0) {
            XAMP_ON_SCOPE_EXIT(LIBAV_LIB.CodecLib->av_packet_unref(packet_.get()));

            ret = LIBAV_LIB.CodecLib->avcodec_receive_packet(codec_context_.get(), packet_.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                return;
            }

            packet_->stream_index = stream_->index;
            if (frame_) {
                packet_->pts = frame_->pts;
                frame_->pts += 100;
            }
            AvIfFailedThrow(LIBAV_LIB.FormatLib->av_write_frame(format_context_.get(), packet_.get()));
        }
    }

    AvEncodeId codec_id_;
    AVCodec* codec_{ nullptr };
    AVStream* stream_{ nullptr };
    AvFileStream input_file_;
    AvPtr<AVFormatContext> format_context_;
    AvPtr<AVCodecContext> codec_context_;
    AvPtr<AVFrame> frame_;
    AvPtr<AVPacket> packet_;
};

AvFileEncoder::AvFileEncoder(AvEncodeId codec_id)
	: impl_(MakeAlign<AvFileEncoderImpl>(codec_id)) {
}

XAMP_PIMPL_IMPL(AvFileEncoder)

void AvFileEncoder::Start(const AnyMap& config) {
    impl_->Start(config);
}

void AvFileEncoder::Encode(std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(progress);
}

}
