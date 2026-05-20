#include <widget/uiplayerstateadapter.h>

#include <base/logger.h>
#include <player/audio_player.h>

#include <QMetaMethod>

UIPlayerStateAdapter::UIPlayerStateAdapter(QObject *parent)
    : QObject(parent)
	, sample_rate_(0)
	, output_buffer_size_(0) {
}

void UIPlayerStateAdapter::OnSampleTime(double stream_time) {
	emit sampleTimeChanged(stream_time);
}

void UIPlayerStateAdapter::OnStateChanged(PlayerState play_state) {
    emit stateChanged(play_state);
}

void UIPlayerStateAdapter::OnError(const std::exception &ex) {
    emit playbackError(ex.what() != nullptr
                                          ? QString::fromStdString(ex.what()) : QString());
}

void UIPlayerStateAdapter::OnDeviceChanged(DeviceState state, const std::string& device_id) {
    emit deviceChanged(state, QString::fromStdString(device_id));
}

void UIPlayerStateAdapter::OnVolumeChanged(int32_t vol) {
    emit volumeChanged(vol);
}

int32_t UIPlayerStateAdapter::sampleRate() const {
	return sample_rate_;
}

size_t UIPlayerStateAdapter::outputBufferSize() const {
	return output_buffer_size_;
}

void UIPlayerStateAdapter::OutputFormatChanged(const AudioFormat output_format, size_t buffer_size) {
	sample_rate_ = static_cast<int32_t>(output_format.GetSampleRate());
	output_buffer_size_ = buffer_size;
	emit outputFormatChanged(sample_rate_, output_buffer_size_);
}

void UIPlayerStateAdapter::OnSamplesChanged(const float* samples, size_t num_buffer_frames) {
	if (samples == nullptr
		|| num_buffer_frames == 0
		|| !isSignalConnected(QMetaMethod::fromSignal(&UIPlayerStateAdapter::samplesChanged))) {
		return;
	}

	std::vector<float> copied_samples(samples, samples + num_buffer_frames);
	emit samplesChanged(std::move(copied_samples), num_buffer_frames);
}
