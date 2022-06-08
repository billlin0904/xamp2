#include <base/math.h>
#include <base/logger.h>
#include <player/audio_player.h>
#include <widget/appsettings.h>
#include <widget/uiplayerstateadapter.h>

using xamp::base::kMaxChannel;
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

void UIPlayerStateAdapter::OnOutputFormatChanged(const AudioFormat output_format, size_t buffer_size) {
	size_t channel_sample_rate = output_format.GetSampleRate() / kMaxChannel;
	size_t frame_size = buffer_size * kMaxChannel;
	size_t shift_size = buffer_size * 0.75; //channel_sample_rate * 0.01;
	frame_size = 8192;//NextPowerOfTwo(buffer_size);
	XAMP_LOG_DEBUG("fft size:{} shift size:{} buffer size:{}", frame_size, shift_size, buffer_size);
	stft_ = MakeAlign<STFT>(frame_size, shift_size);
	stft_->setWindowType(AppSettings::getAsEnum<WindowType>(kAppSettingWindowType));
}

void UIPlayerStateAdapter::OnSamplesChanged(const float* samples, size_t num_buffer_frames) {
	emit fftResultChanged(stft_->Process(samples, num_buffer_frames));
}