#include <player/audio_player.h>
#include <widget/uiplayerstateadapter.h>

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

void UIPlayerStateAdapter::OnDisplayChanged(std::vector<float> const& display) {
    emit displayChanged(display);
}