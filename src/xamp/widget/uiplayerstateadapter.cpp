#include <player/audio_player.h>
#include <widget/uiplayerstateadapter.h>

using xamp::player::AudioPlayer;

UIPlayerStateAdapter::UIPlayerStateAdapter(QObject *parent)
    : QObject(parent)
    , play_queue_(kPlayQueueSize)
    , index_queue_(kPlayQueueSize) {
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

void UIPlayerStateAdapter::OnDeviceChanged(xamp::output_device::DeviceState state) {
    emit deviceChanged(state);
}

void UIPlayerStateAdapter::OnVolumeChanged(float vol) {
    emit volumeChanged(vol);
}

void UIPlayerStateAdapter::OnSampleDataChanged(const float *, size_t) {
}

void UIPlayerStateAdapter::OnGaplessPlayback() {
    auto index = popIndexQueue();
    emit gaplessPlayback(index);
}

size_t UIPlayerStateAdapter::GetPlayQueueSize() const {
    return play_queue_.size();
}

void UIPlayerStateAdapter::addPlayQueue(const std::wstring &file_ext, const std::wstring &file_path, const QModelIndex &index) {
    auto stream = AudioPlayer::MakeFileStream(file_ext);
    stream->OpenFile(file_path);
    play_queue_.TryEnqueue(std::move(stream));
    index_queue_.TryPush(index);
}

QModelIndex UIPlayerStateAdapter::popIndexQueue() {
    auto index = *index_queue_.Front();
    index_queue_.Pop();
    return index;
}

AlignPtr<FileStream>& UIPlayerStateAdapter::PlayQueueFont() {
    return *play_queue_.Front();
}

void UIPlayerStateAdapter::PopPlayQueue() {
    play_queue_.Pop();
}

void UIPlayerStateAdapter::ClearPlayQueue() {
    play_queue_.clear();
    index_queue_.clear();
}