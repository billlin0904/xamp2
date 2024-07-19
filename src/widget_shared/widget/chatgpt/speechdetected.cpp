#include <widget/chatgpt/speechdetected.h>

SpeechDetected::SpeechDetected(QObject *parent)
    : QObject(parent) {
}

void SpeechDetected::reset() {
    _voice_buffer.clear();
    setVoiceInProgress(false);
    _segment_approved         = false;
    _patience_counter         = _params.patience;
    _detected_samples_counter = _params.minimum_samples;
}

void SpeechDetected::adjust(const std::vector<float>& data) {
    auto energy = std::inner_product(data.begin(), data.end(), data.begin(), 0.0f) / data.size();
    auto diff   = std::abs(energy - _mean_energy);

    _mean_energy = _mean_energy * _params.beta + (1 - _params.beta) * energy;
    _std_energy  = _std_energy * _params.beta + (1 - _params.beta) * diff;
}

float SpeechDetected::threshold() const {
    auto lambda = 1 / _mean_energy;
    return 2 * std::log(10) / lambda;
}

void SpeechDetected::setAdjustInProgress(bool progress) {
    adjustInProgress = progress;
}

void SpeechDetected::setVoiceInProgress(bool progress) {
    voiceInProgress = progress;
}

bool SpeechDetected::getVoiceInProgress() {
    return voiceInProgress;
}

void SpeechDetected::feedSamples(const std::vector<float> &data) {
    _adjustment_counter = std::max(_adjustment_counter - 1, 0);
    if (_adjustment_counter > 0) {
        setAdjustInProgress(true);
        adjust(data);
        return;
    }
    setAdjustInProgress(false);

    auto energy = std::inner_product(data.begin(), data.end(), data.begin(), 0.0f) / data.size();
    const bool current_score = energy > threshold();

    if (current_score) {
        // reset patience
        _patience_counter = _params.patience;

        // start the new potential segment if not already started
        setVoiceInProgress(true);

        // count consecutive accepted samples
        if (--_detected_samples_counter < 0) {
            _segment_approved = true;
        }
    } else {
        // decrement patience counter
        _patience_counter = std::max(_patience_counter - 1, 0);

        // reset accepted samples counter
        _detected_samples_counter = _params.minimum_samples;
    }

    // Capture voice if speech is detected
    if (getVoiceInProgress()) {
        _voice_buffer.insert(_voice_buffer.end(), data.begin(), data.end());
    }


    // if patience runs out, signal speech detection and reset buffers
    if (_patience_counter <= 0 && getVoiceInProgress()) {
        if (_segment_approved) {
            emit speechDetected(_voice_buffer);
        }
        reset();
    }
}
