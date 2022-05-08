#include <base/math.h>
#include <player/audio_player.h>
#include <widget/uiplayerstateadapter.h>

using xamp::base::NextPowerOfTwo;

UIPlayerStateAdapter::UIPlayerStateAdapter(QObject *parent)
    : QObject(parent) {
}

void UIPlayerStateAdapter::OnSampleTime(double stream_time) {
	emit sampleTimeChanged(stream_time);
}

void UIPlayerStateAdapter::OnStateChanged(PlayerState play_state) {
    emit stateChanged(play_state);
}

void UIPlayerStateAdapter::OnError(const Exception &ex) {
    emit playbackError(ex.GetError(), ex.what() != nullptr
                                          ? QString::fromStdString(ex.what()) : QString());
}

void UIPlayerStateAdapter::OnDeviceChanged(DeviceState state) {
    emit deviceChanged(state);
}

void UIPlayerStateAdapter::OnVolumeChanged(float vol) {
    emit volumeChanged(vol);
}

void UIPlayerStateAdapter::OnOutputFormatChanged(const AudioFormat output_format) {
	size_t channels = output_format.GetChannels();
	size_t channel_sample_rate = output_format.GetSampleRate() / 2;
	size_t frame_size = channel_sample_rate * 0.018;
	size_t shift_size = channel_sample_rate * 0.01;
	frame_size = NextPowerOfTwo(frame_size);
	stft_ = MakeAlign<STFT>(channels, frame_size, shift_size);
}

void UIPlayerStateAdapter::OnSamplesChanged(const float* samples, size_t num_buffer_frames) {
	emit fftResultChanged(stft_->Process(samples, num_buffer_frames));
}