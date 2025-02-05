#include <widget/uiplayerstateadapter.h>

#include <base/math.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <player/audio_player.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>

UIPlayerStateAdapter::UIPlayerStateAdapter(QObject *parent)
    : QObject(parent)
	, enable_spectrum_(false)
	, band_size_(128)
	, fft_size_(8192)
	, desired_band_width_(44100.0) {
}

void UIPlayerStateAdapter::setBandSize(size_t band_size) {
	band_size_ = band_size;
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

size_t UIPlayerStateAdapter::fftSize() const {
	return fft_size_;
}

void UIPlayerStateAdapter::OutputFormatChanged(const AudioFormat output_format, size_t buffer_size) {
	enable_spectrum_ = qAppSettings.valueAsBool(kAppSettingEnableSpectrum);
	if (!enable_spectrum_) {
		return;
	}
	stft_.reset();
	if (output_format.GetSampleRate() > 48000) {
		return;
	}
	size_t fft_shift_size = buffer_size * 0.55;	
	size_t frame_size = 0;
	fft_size_ = 4096;
	if (buffer_size > fft_size_) {
		stft_.reset();
		return;
	}
	frame_size = fft_size_ * AudioFormat::kMaxChannel;
	XAMP_LOG_DEBUG("fft size:{} shift size:{} buffer size:{}", frame_size, fft_shift_size, buffer_size);
	stft_ = MakeAlign<STFT>(frame_size, fft_shift_size);
	stft_->SetWindowType(WindowType::HAMMING);
}

void UIPlayerStateAdapter::OnSamplesChanged(const float* samples, size_t num_buffer_frames) {
	if (!enable_spectrum_ || !stft_) {
		return;
	}
#ifndef _DEBUG
	emit fftResultChanged(stft_->Process(samples, num_buffer_frames));
#endif
}