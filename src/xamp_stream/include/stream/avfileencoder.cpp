#include <stream/avfileencoder.h>
#include <stream/avfilestream.h>

#include <fstream>

#include <stream/avlib.h>

#include <base/dsd_utils.h>
#include <base/scopeguard.h>
#include <base/encodingprofile.h>

#ifdef USE_AV_ENCODER
XAMP_STREAM_NAMESPACE_BEGIN

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
        AvIfFailedThrow(LIBAV_LIB.FormatLib->avformat_alloc_output_context2(&format_ctx, nullptr, nullptr, output_file.c_str()));
        AvIfFailedThrow(LIBAV_LIB.FormatLib->avio_open(&format_ctx->pb, output_file.c_str(), AVIO_FLAG_WRITE));        
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
        case AvEncodeId::AV_ENCODE_ID_ALAC:
            id = AVCodecID::AV_CODEC_ID_ALAC;
            break;
        default: 
            break;
        }

        format_context_.reset(format_ctx);
        stream_ = LIBAV_LIB.FormatLib->avformat_new_stream(format_context_.get(), nullptr);

        codec_ = LIBAV_LIB.CodecLib->avcodec_find_encoder(id);
        if (!codec_) {
            return;
        }
        
        stream_->codec->bit_rate = encoding_profile.bitrate * 1000;
        stream_->codec->sample_fmt = AV_SAMPLE_FMT_FLTP;
        stream_->codec->codec_type = AVMEDIA_TYPE_AUDIO;
        stream_->codec->sample_rate = encoding_profile.sample_rate;
        stream_->codec->channel_layout = AV_CH_LAYOUT_STEREO;
        stream_->codec->channels = LIBAV_LIB.UtilLib->av_get_channel_layout_nb_channels(stream_->codec->channel_layout);

        if (!CheckSampleRate(codec_, stream_->codec->sample_rate)) {
            return;
        }
        if (!CheckSampleFmt(codec_, stream_->codec->sample_fmt)) {
            return;
        }  

        AvIfFailedThrow(LIBAV_LIB.CodecLib->avcodec_open2(stream_->codec, codec_, nullptr));

        frame_.reset(LIBAV_LIB.UtilLib->av_frame_alloc());
        frame_->nb_samples = stream_->codec->frame_size;
        frame_->format = stream_->codec->sample_fmt;
        frame_->channel_layout = stream_->codec->channel_layout;
        
        AvIfFailedThrow(LIBAV_LIB.UtilLib->av_frame_get_buffer(frame_.get(), 0));
        auto buffer_size = LIBAV_LIB.UtilLib->av_get_bytes_per_sample((AVSampleFormat)frame_->format)
            * frame_->channels
            * frame_->nb_samples;
        pcm_buf_.resize(buffer_size * sizeof(float));
        pts_ = 0;
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
        AvIfFailedThrow(LIBAV_LIB.UtilLib->av_frame_make_writable(frame_.get()));
        packet_.reset(LIBAV_LIB.CodecLib->av_packet_alloc());
        AvIfFailedThrow(LIBAV_LIB.FormatLib->avformat_write_header(format_context_.get(), nullptr));

        while (true) {
            auto read_bytes = input_file_.GetSamples(pcm_buf_.data(), pcm_buf_.size());
            if (!read_bytes) {
                break;
            }
            AvIfFailedThrow(LIBAV_LIB.UtilLib->av_frame_make_writable(frame_.get()));
            LIBAV_LIB.UtilLib->av_samples_fill_arrays(frame_->data,
                frame_->linesize,
                pcm_buf_.data(),
                frame_->channels,
                frame_->nb_samples, 
                (AVSampleFormat)frame_->format,
                0);
            packet_->stream_index = stream_->index;
            EncodeToFile(progress);
        }
    }

private:
    void EncodeToFile(std::function<bool(uint32_t) > const& progress) {     
        auto codec_context = stream_->codec;

        AvIfFailedThrow(LIBAV_LIB.CodecLib->avcodec_send_frame(codec_context, frame_.get()));

        while (true) {            
            XAMP_ON_SCOPE_EXIT(LIBAV_LIB.CodecLib->av_packet_unref(packet_.get()));
            auto ret = LIBAV_LIB.CodecLib->avcodec_receive_packet(codec_context, packet_.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                return;
            }
            AvIfFailedThrow(ret);
            packet_->pos = -1;
            LIBAV_LIB.CodecLib->av_packet_rescale_ts(packet_.get(), codec_context->time_base, format_context_->streams[0]->time_base);
            if (packet_->pts <= pts_) {
                packet_->pts = pts_ + 1;
                packet_->dts = pts_ + 1;
            }
            pts_ = packet_->pts;
            AvIfFailedThrow(LIBAV_LIB.FormatLib->av_interleaved_write_frame(format_context_.get(), packet_.get()));
        }
    }

    int64_t pts_{ 0 };
    AvEncodeId codec_id_;
    AVCodec* codec_{ nullptr };
    AVStream* stream_{ nullptr };    
    AvFileStream input_file_;
    std::vector<uint8_t> pcm_buf_;
    std::ofstream file_;
    AvPtr<AVFormatContext> format_context_;
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

XAMP_STREAM_NAMESPACE_END
#endif