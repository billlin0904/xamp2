#include <base/stopwatch.h>
#include <webrtc/webrtc.hpp>

#include <widget/chatgpt/speechdetected.h>

class SpeechDetected::SpeechDetectedImpl {
public:
    SpeechDetectedImpl(int32_t sample_rate)
        : vad_(webrtc::Vad::kVadAggressive)
        , sample_rate_(sample_rate) {
        auto result = vad_.Init();
    }

    bool isSpeech(const std::vector<float>& data) {
        std::vector<int16_t> temp;
		temp.resize(data.size());
		for (int i = 0; i < data.size(); i++) {
			temp[i] = static_cast<int16_t>(data[i] * 32768.0f);
		}
        return vad_.IsSpeech(temp.data(), temp.size(), sample_rate_) == webrtc::Vad::kActive;
    }

private:
    int32_t sample_rate_;
    webrtc::Vad vad_;
};

SpeechDetected::SpeechDetected(QObject *parent)
    : QObject(parent)
    , impl_(MakeAlign<SpeechDetectedImpl>(16000)) {
}

SpeechDetected::~SpeechDetected() {
}

bool SpeechDetected::isSpeech(const std::vector<float>& data) {
    return impl_->isSpeech(data);
}

void SpeechDetected::reset() {
    
}