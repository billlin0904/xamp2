#include <Gist.h>
#include <player/spectrumanalyser.h>

namespace xamp::player {

class SpectrumAnalyser::SpectrumAnalyserImpl {
public:
	SpectrumAnalyserImpl()
		: gitst_(1024, 1024) {
	}

	const std::vector<float>& GeMagnitude() {
		return gitst_.getMagnitudeSpectrum();
	}

	bool Process(AudioBuffer<float>& buffer) {
		if (buffer.TryRead(buffer_.data(), buffer_.size())) {
			(void)FastMemcpy(left_channel_.data(), buffer_.data(), left_channel_.size());
			(void)FastMemcpy(right_channel_.data(), buffer_.data() + left_channel_.size(), right_channel_.size());
			for (auto i = 0; i < left_channel_.size(); ++i) {
				marge_channel_[i] = (left_channel_[i] + right_channel_[i]) / 2;
			}
			gitst_.processAudioFrame(marge_channel_);
			return true;
		}
		return false;
	}

	void SetAudioFrameSize(int32_t frame_size) {
		buffer_.resize(frame_size);
		left_channel_.resize(frame_size / 2);
		right_channel_.resize(frame_size / 2);
		marge_channel_.resize(frame_size / 2);
		gitst_.setAudioFrameSize(left_channel_.size());	
	}

	void SetSamplingFrequency(int32_t fs) {
		gitst_.setSamplingFrequency(fs);
	}
private:
	Gist<float> gitst_;
	std::vector<float> buffer_;
	std::vector<float> marge_channel_;
	std::vector<float> left_channel_;
	std::vector<float> right_channel_;
};

SpectrumAnalyser::SpectrumAnalyser()
	: impl_(MakeAlign<SpectrumAnalyserImpl>()) {
}

SpectrumAnalyser::~SpectrumAnalyser() {
}

void SpectrumAnalyser::SetAudioFrameSize(int32_t frame_size) {
	impl_->SetAudioFrameSize(frame_size);
}

void SpectrumAnalyser::SetSamplingFrequency(int32_t fs) {
	impl_->SetSamplingFrequency(fs);
}

const std::vector<float>& SpectrumAnalyser::GeMagnitude() {
	return impl_->GeMagnitude();
}

bool SpectrumAnalyser::Process(AudioBuffer<float>& buffer) {
	return impl_->Process(buffer);
}

}
