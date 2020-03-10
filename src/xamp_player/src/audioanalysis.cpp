#include <Gist.h>
#include <player/audioanalysis.h>

namespace xamp::player {

class AudioAnalysis::AudioAnalysisImpl {
public:
	AudioAnalysisImpl()
		: gist_(1024, 1024) {
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

	void SetAudioFrameSize(int32_t frame_size) {
		buffer_.resize(frame_size);
		gist_.setAudioFrameSize(frame_size);
	}

	void SetSamplingFrequency(int32_t fs) {
		gist_.setSamplingFrequency(fs);
	}
private:
	Gist<float> gist_;
	std::vector<float> buffer_;
};

AudioAnalysis::AudioAnalysis()
	: impl_(MakeAlign<AudioAnalysisImpl>()) {
}

AudioAnalysis::~AudioAnalysis() {
}

void AudioAnalysis::SetAudioFrameSize(int32_t frame_size) {
	impl_->SetAudioFrameSize(frame_size);
}

void AudioAnalysis::SetSamplingFrequency(int32_t fs) {
	impl_->SetSamplingFrequency(fs);
}

const std::vector<float>& AudioAnalysis::GetMagnitudeSpectrum() {
	return impl_->GetMagnitudeSpectrum();
}

bool AudioAnalysis::Process(AudioBuffer<float>& buffer) {
	return impl_->Process(buffer);
}

}
