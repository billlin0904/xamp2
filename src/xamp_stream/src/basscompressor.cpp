#include <stream/basslib.h>
#include <base/memory.h>
#include <base/buffer.h>
#include <stream/basscompressor.h>

namespace xamp::stream {

class BassCompressor::BassCompressorImpl {
public:
    BassCompressorImpl() {
        logger_ = Logger::GetInstance().GetLogger(kCompressorLoggerName);
    }

    void Start(uint32_t samplerate) {
        stream_.reset(BASS.BASS_StreamCreate(samplerate,
                                             kMaxChannel,
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

    void Process(float const * samples, uint32_t num_samples, Buffer<float>& out) {
        if (out.size() < num_samples) {
            out.resize(num_samples);
    	}        
        MemoryCopy(out.data(), samples, num_samples * sizeof(float));

        const auto bytes_read = 
            BASS.BASS_ChannelGetData(stream_.get(),
                out.data(),
                num_samples * sizeof(float));
        if (bytes_read == kBassError) {
            return;
        }
        if (bytes_read == 0) {
            return;
        }
        const auto frames = bytes_read / sizeof(float);
        out.resize(frames);
    }

private:
    BassStreamHandle stream_;
    ::BASS_BFX_COMPRESSOR2 compressord_{0};
    std::shared_ptr<spdlog::logger> logger_;
};

BassCompressor::BassCompressor()
    : impl_(MakeAlign<BassCompressorImpl>()) {
}

void BassCompressor::Start(uint32_t samplerate) {
    impl_->Start(samplerate);
}

XAMP_PIMPL_IMPL(BassCompressor)

void BassCompressor::Init(CompressorParameters const &parameters) {
    impl_->Init(parameters);
}

void BassCompressor::Process(float const * samples, uint32_t num_samples, Buffer<float>& out) {
    return impl_->Process(samples, num_samples, out);
}

Uuid BassCompressor::GetTypeId() const {
    return Id;
}

}

