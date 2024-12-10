#include <stream/basscompressor.h>

#include <stream/compressorconfig.h>
#include <stream/bass_util.h>
#include <stream/basslib.h>

#include <base/buffer.h>
#include <base/logger_impl.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(BassCompressor);

class BassCompressor::BassCompressorImpl {
public:
    BassCompressorImpl() {
        logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(BassCompressor));
    }

    void Start(uint32_t output_sample_rate) {
        stream_.reset(BASS_LIB.BASS_StreamCreate(output_sample_rate,
                                             AudioFormat::kMaxChannel,
                                             BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                                             STREAMPROC_DUMMY,
                                             nullptr));
        BassIfFailedThrow(stream_);
    }

    void Initialize(const CompressorConfig& config) {
        ::BASS_BFX_COMPRESSOR2 compressord{0};
        compressord.fGain      = config.gain;
        compressord.fThreshold = config.threshold;
        compressord.fRatio     = config.ratio;
        compressord.fAttack    = config.attack;
        compressord.fRelease   = config.release;
        compressord.lChannel   = BASS_BFX_CHANALL;
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

    bool Process(float const * samples, size_t num_samples, BufferRef<float>& out) const {
		return bass_util::ReadStream(stream_, samples, num_samples, out);
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
    return Description;
}

XAMP_STREAM_NAMESPACE_END

