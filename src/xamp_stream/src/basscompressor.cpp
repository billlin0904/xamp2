#include <stream/basscompressor.h>

#include <stream/compressorconfig.h>
#include <stream/bass_utiltis.h>
#include <stream/basslib.h>

#include <base/buffer.h>
#include <base/logger_impl.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(BassCompressor);

class BassCompressor::BassCompressorImpl {
public:
    BassCompressorImpl() {
        logger_ = XampLoggerFactory.GetLogger(kBassCompressorLoggerName);
    }

    void Start(uint32_t output_sample_rate) {
        stream_.reset(BASS_LIB.BASS_StreamCreate(output_sample_rate,
                                             AudioFormat::kMaxChannel,
                                             BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                                             STREAMPROC_DUMMY,
                                             nullptr));
        BassIfFailedThrow(stream_);
    }

    void Initialize(CompressorConfig const& parameters) {
        ::BASS_BFX_COMPRESSOR2 compressord{0};
        compressord.fGain = parameters.gain;
        compressord.fThreshold = parameters.threshold;
        compressord.fRatio = parameters.ratio;
        compressord.fAttack = parameters.attack;
        compressord.fRelease = parameters.release;
        compressord.lChannel = BASS_BFX_CHANALL;
        const auto compressor_fx = BASS_LIB.BASS_ChannelSetFX(
            stream_.get(),
            BASS_FX_BFX_COMPRESSOR2,
            0);
        BassIfFailedThrow(compressor_fx);
        BassIfFailedThrow(BASS_LIB.BASS_FXSetParameters(compressor_fx, &compressord));
        XAMP_LOG_D(logger_, "Compressor gain:{} threshold:{} ratio:{} attack:{} release:{}",
            compressord.fGain,
            compressord.fThreshold,
            compressord.fRatio,
            compressord.fAttack,
            compressord.fRelease);
    }

    bool Process(float const * samples, size_t num_samples, BufferRef<float>& out) {
        if (out.size() != num_samples) {
            out.maybe_resize(num_samples);
        }
        MemoryCopy(out.data(), samples, num_samples * sizeof(float));

        const auto bytes_read =
            BASS_LIB.BASS_ChannelGetData(stream_.get(),
                out.data(),
                num_samples * sizeof(float));
        if (bytes_read == kBassError) {
            return false;
        }
        if (bytes_read == 0) {
            return false;
        }
        const auto frames = bytes_read / sizeof(float);
        out.maybe_resize(frames);
        return true;
    }

private:
    BassStreamHandle stream_;
    LoggerPtr logger_;
};

BassCompressor::BassCompressor()
    : impl_(MakeAlign<BassCompressorImpl>()) {
}

XAMP_PIMPL_IMPL(BassCompressor)

void BassCompressor::Initialize(const AnyMap& config) {
    const auto output_format = config.AsAudioFormat(DspConfig::kOutputFormat);
    impl_->Start(output_format.GetSampleRate());

	const auto compressor_config = config.Get<CompressorConfig>(DspConfig::kCompressorConfig);
    impl_->Initialize(compressor_config);
}

bool BassCompressor::Process(float const * samples, size_t num_samples, BufferRef<float>& out) {
    return impl_->Process(samples, num_samples, out);
}

Uuid BassCompressor::GetTypeId() const {
    return XAMP_UUID_OF(BassCompressor);
}

std::string_view BassCompressor::GetDescription() const noexcept {
    return "BassCompressor";
}

XAMP_STREAM_NAMESPACE_END

