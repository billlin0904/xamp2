#include <Gist.h>
#include <player/audioanalysis.h>

namespace xamp::player {

class AudioAnalysis::AudioAnalysisImpl {
public:
	AudioAnalysisImpl(int32_t frame_size, int32_t samplerate) 
		: gist_(frame_size, samplerate)
		, buffer_(frame_size) {
	}

	const std::vector<float>& GetMagnitudeSpectrum() {
		return gist_.getMagnitudeSpectrum();
	}

	bool Process(AudioBuffer<float>& buffer) {
		if (buffer.TryRead(buffer_.data(), buffer_.size())) {
			gist_.processAudioFrame(buffer_);
			return true;
		}
		return false;
	}
private:
	Gist<float> gist_;
	std::vector<float> buffer_;
};

AudioAnalysis::AudioAnalysis(int32_t frame_size, int32_t samplerate)
	: impl_(MakeAlign<AudioAnalysisImpl>(frame_size, samplerate)) {
}

AudioAnalysis::~AudioAnalysis() {
}

const std::vector<float>& AudioAnalysis::GetMagnitudeSpectrum() {
	return impl_->GetMagnitudeSpectrum();
}

bool AudioAnalysis::Process(AudioBuffer<float>& buffer) {
	return impl_->Process(buffer);
}

}
