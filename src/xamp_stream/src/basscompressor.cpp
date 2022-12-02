#include <stream/basslib.h>
#include <base/memory.h>
#include <base/buffer.h>
#include <base/logger_impl.h>
#include <stream/bass_utiltis.h>
#include <stream/basscompressor.h>

namespace xamp::stream {

class BassCompressor::BassCompressorImpl {
public:
    BassCompressorImpl() {
        logger_ = LoggerManager::GetInstance().GetLogger(kCompressorLoggerName);
    }

    void Start(uint32_t output_sample_rate) {
        stream_.reset(BASS.BASS_StreamCreate(output_sample_rate,
                                             AudioFormat::kMaxChannel,
                                             BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                                             STREAMPROC_DUMMY,
                                             nullptr));
        MemorySet(&compressord_, 0, sizeof(compressord_));
    }

    void Init(CompressorParameters const& parameters) {
        ::BASS_BFX_COMPRESSOR2 compressord{0};
        compressord.fGain = parameters.gain;
        compressord.fThreshold = parameters.threshold;
        compressord.fRatio = parameters.ratio;
        compressord.fAttack = parameters.attack;
        compressord.fRelease = parameters.release;
        compressord.lChannel = BASS_BFX_CHANALL;
        const auto compressor_fx = BASS.BASS_ChannelSetFX(
            stream_.get(),
            BASS_FX_BFX_COMPRESSOR2,
            0);
        BassIfFailedThrow(compressor_fx);
        BassIfFailedThrow(BASS.BASS_FXSetParameters(compressor_fx, &compressord));
        compressord_ = compressord;
        XAMP_LOG_D(logger_, "Compressor gain:{} threshold:{} ratio:{} attack:{} release:{}",
            compressord.fGain,
            compressord.fThreshold,
            compressord.fRatio,
            compressord.fAttack,
            compressord.fRelease);
    }

    bool Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) {
        return BassUtiltis::Process(stream_, samples, num_samples, out);
    }

private:
    BassStreamHandle stream_;
    ::BASS_BFX_COMPRESSOR2 compressord_{0};
    std::shared_ptr<Logger> logger_;
};

BassCompressor::BassCompressor()
    : impl_(MakeAlign<BassCompressorImpl>()) {
}

void BassCompressor::Start(const DspConfig& config) {
    const auto output_format = config.Get<AudioFormat>(DspConfig::kOutputFormat);
    impl_->Start(output_format.GetSampleRate());
}

XAMP_PIMPL_IMPL(BassCompressor)

void BassCompressor::Init(const DspConfig& config) {
	const auto parameters = config.Get<CompressorParameters>(DspConfig::kCompressorParameters);
    impl_->Init(parameters);
}

bool BassCompressor::Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) {
    return impl_->Process(samples, num_samples, out);
}

Uuid BassCompressor::GetTypeId() const {
    return UuidOf<BassCompressor>::Id();
}

std::string_view BassCompressor::GetDescription() const noexcept {
    return "Compressor";
}

void BassCompressor::Flush() {
	
}


}

