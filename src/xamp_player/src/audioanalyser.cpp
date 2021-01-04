#include <Gist.h>
#include <CoreTimeDomainFeatures.h>
#include <CoreFrequencyDomainFeatures.h>

#include <algorithm>
#include <player/audioanalyser.h>

namespace xamp::player {

class AudioAnalyser::AudioAnalyserImpl final {
public:
    AudioAnalyserImpl(int32_t frame_size, int32_t sample_rate)
        : gist_(frame_size, sample_rate) {
    }

    void Process(float const * samples, uint32_t num_sample) {
        memcpy(buffer_.data(), samples, std::min(buffer_.size(), static_cast<size_t>(num_sample)));
        gist_.processAudioFrame(buffer_);
    }

    float GetRMS() {
        return time_domain_.rootMeanSquare(gist_.getMagnitudeSpectrum());
    }

    float GetPeakEnergy() {
        return time_domain_.peakEnergy(gist_.getMagnitudeSpectrum());
    }

    float GetZeroCrossingRate() {
        return time_domain_.zeroCrossingRate(gist_.getMagnitudeSpectrum());
    }

    const std::vector<float>& GetMelFrequencySpectrum() {
        return gist_.getMelFrequencySpectrum();
    }

    const std::vector<float> & GetMagnitudeSpectrum() {
        return gist_.getMagnitudeSpectrum();
    }

    const std::vector<float>& GetMelFrequencyCepstralCoefficients() {
        return gist_.getMelFrequencyCepstralCoefficients();
    }
private:
    Gist<float> gist_;
    CoreTimeDomainFeatures<float> time_domain_;
    std::vector<float> buffer_;
};

AudioAnalyser::AudioAnalyser(int32_t frame_size, int32_t sample_rate)
    : impl_(MakeAlign<AudioAnalyserImpl>(frame_size, sample_rate)) {
}

AudioAnalyser::~AudioAnalyser() {
}

void AudioAnalyser::Process(float const * samples, uint32_t num_sample) {
    return impl_->Process(samples, num_sample);
}

float AudioAnalyser::GetRMS() {
    return impl_->GetRMS();
}

float AudioAnalyser::GetPeakEnergy() {
    return impl_->GetPeakEnergy();
}

float AudioAnalyser::GetZeroCrossingRate() {
     return impl_->GetZeroCrossingRate();
}

}
