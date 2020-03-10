#include <Gist.h>
#include <player/audioanalysis.h>

namespace xamp::player {

class AudioAnalysis::AudioAnalysisImpl {
public:
	AudioAnalysisImpl(int32_t frame_size, int32_t samplerate) 
		: gist_(frame_size, samplerate) {
	}

	const std::vector<float>& GetMagnitudeSpectrum() {
		return gist_.getMagnitudeSpectrum();
	}

	void Process(const std::vector<float>& frame) {
		gist_.processAudioFrame(frame);
	}
private:
	Gist<float> gist_;
};

AudioAnalysis::AudioAnalysis(int32_t frame_size, int32_t samplerate)
	: impl_(MakeAlign<AudioAnalysisImpl>(frame_size, samplerate)) {
}

AudioAnalysis::~AudioAnalysis() {
}

const std::vector<float>& AudioAnalysis::GetMagnitudeSpectrum() {
	return impl_->GetMagnitudeSpectrum();
}

void AudioAnalysis::Process(const std::vector<float>& frame) {
	impl_->Process(frame);
}

}
