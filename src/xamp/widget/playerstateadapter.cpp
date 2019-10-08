#include "playerstateadapter.h"

PlayerStateAdapter::PlayerStateAdapter(QObject *parent)
    : QObject(parent) {
}

void PlayerStateAdapter::OnSampleTime(double stream_time) {
	emit sampleTimeChanged(stream_time);
}

void PlayerStateAdapter::OnStateChanged(xamp::player::PlayerState play_state) {
    emit stateChanged(play_state);
}

void PlayerStateAdapter::OnError(const xamp::base::Exception &ex) {
    emit playbackError(ex.GetError(), ex.what() != nullptr ? QString::fromStdString(ex.what()) : QString());
}