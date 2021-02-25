#include <stream/basslib.h>
#include <base/memory.h>
#include <stream/compressor.h>

namespace xamp::stream {

class Compressor::CompressorImpl {
public:
    void SetSampleRate(uint32_t sample_rate) {
        stream_.reset(BASS.BASS_StreamCreate(sample_rate,
                                             kMaxChannel,
                                             BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                                             STREAMPROC_DUMMY,
                                             nullptr));
        MemorySet(&compressord_, 0, sizeof(compressord_));
    }

    void Prepare(CompressorParameters const& parameters) {
        ::BASS_BFX_COMPRESSOR2 compressord;
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
    }

    const std::vector<float>& Process(float const * samples, uint32_t num_samples) {
        buffer_.resize(num_samples);
        MemoryCopy(buffer_.data(), samples, num_samples * sizeof(float));

        const auto bytes_read = BASS.BASS_ChannelGetData(stream_.get(),
                                                         buffer_.data(),
                                                         buffer_.size() * sizeof(float));
        if (bytes_read == kBassError) {
            return buffer_;
        }
        if (bytes_read == 0) {
            return buffer_;
        }
        const auto frames = bytes_read / sizeof(float);
        buffer_.resize(frames);
        return buffer_;
    }

private:
    BassStreamHandle stream_;
    ::BASS_BFX_COMPRESSOR2 compressord_{0};
    std::vector<float> buffer_;
};

Compressor::Compressor()
    : impl_(MakeAlign<CompressorImpl>()) {
}

void Compressor::SetSampleRate(uint32_t sample_rate) {
    impl_->SetSampleRate(sample_rate);
}

XAMP_PIMPL_IMPL(Compressor)

void Compressor::Prepare(CompressorParameters const &parameters) {
    impl_->Prepare(parameters);
}

const std::vector<float>& Compressor::Process(float const * samples, uint32_t num_samples) {
    return impl_->Process(samples, num_samples);
}

Uuid Compressor::GetTypeId() const {
    return Id;
}

}

