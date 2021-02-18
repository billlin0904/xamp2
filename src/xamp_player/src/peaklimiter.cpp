#include <stream/basslib.h>
#include <base/memory.h>
#include <player/peaklimiter.h>

namespace xamp::player {

using namespace xamp::stream;

class PeakLimiter::PeakLimiterImpl {
public:
    PeakLimiterImpl() {
    }

    void SetSampleRate(uint32_t output_sample_rate) {
        stream_.reset(BASS.BASS_StreamCreate(output_sample_rate,
                                             kMaxChannel,
                                             BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                                             STREAMPROC_DUMMY,
                                             nullptr));
    }

    void Setup(float gain, float threshold, float ratio, float attack, float release) {
        ::BASS_BFX_COMPRESSOR2 compressord{0};
        compressord.fGain = gain;
        compressord.fThreshold = threshold;
        compressord.fRatio = ratio;
        compressord.fAttack = attack;
        compressord.fRelease = release;
        compressord.lChannel = BASS_BFX_CHANALL;
        auto compressor_fx = BASS.BASS_ChannelSetFX(
            stream_.get(),
            BASS_FX_BFX_COMPRESSOR2,
            0);
        BassIfFailedThrow(compressor_fx);
        BassIfFailedThrow(BASS.BASS_FXSetParameters(compressor_fx, &compressord));
    }

    const std::vector<float>& Process(float const * samples, uint32_t num_samples) {
        result_.clear();

        buffer_.resize(num_samples);
        MemoryCopy(buffer_.data(), samples, num_samples * sizeof(float));

        const auto bytes_read = BASS.BASS_ChannelGetData(stream_.get(),
                                                         buffer_.data(),
                                                         buffer_.size() * sizeof(float));
        if (bytes_read == kBassError) {
            return result_;
        }
        else if (bytes_read == 0) {
            return result_;
        }
        auto frames = bytes_read / sizeof(float);
        buffer_.resize(frames);
        result_.insert(result_.end(), buffer_.begin(), buffer_.end());
        return result_;
    }

    BassStreamHandle stream_;
    std::vector<float> result_;
    std::vector<float> buffer_;
};

PeakLimiter::PeakLimiter()
    : impl_(MakeAlign<PeakLimiterImpl>()) {
}

void PeakLimiter::SetSampleRate(uint32_t output_sample_rate) {
    impl_->SetSampleRate(output_sample_rate);
}

XAMP_PIMPL_IMPL(PeakLimiter)

void PeakLimiter::Setup(float gain, float threshold, float ratio, float attack, float release) {
    impl_->Setup(gain, threshold, ratio, attack, release);
}

const std::vector<float>& PeakLimiter::Process(float const * samples, uint32_t num_samples) {
    return impl_->Process(samples, num_samples);
}

}

