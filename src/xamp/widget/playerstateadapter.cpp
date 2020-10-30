#include <player/audio_player.h>
#include <widget/playerstateadapter.h>

PlayerStateAdapter::PlayerStateAdapter(QObject *parent)
    : QObject(parent)
    , next_playindex_(100)
    , next_streams_(100) {
}

void PlayerStateAdapter::addPlaybackQueue(const QModelIndex& next_index, const std::wstring& file_ext, const std::wstring& file_path) {
    auto stream = AudioPlayer::MakeFileStream(file_ext);
    stream->OpenFile(file_path);
    next_streams_.Enqueue(std::move(stream));
    next_playindex_.Push(next_index);
}

void PlayerStateAdapter::clearPlaybackQueue() {
}

size_t PlayerStateAdapter::getPlaybackQueueSize() const {
    return next_playindex_.size();
}

void PlayerStateAdapter::OnSampleTime(double stream_time, double sample_time) {
	emit sampleTimeChanged(stream_time, sample_time);
}

void PlayerStateAdapter::OnStateChanged(PlayerState play_state) {
    emit stateChanged(play_state);
}

void PlayerStateAdapter::OnError(const Exception &ex) {
    emit playbackError(ex.GetError(), ex.what() != nullptr ? QString::fromStdString(ex.what()) : QString());
}

void PlayerStateAdapter::OnDeviceChanged(xamp::output_device::DeviceState state) {
    emit deviceChanged(state);
}

void PlayerStateAdapter::OnVolumeChanged(float vol) {
    emit volumeChanged(vol);
}

void PlayerStateAdapter::OnSampleDataChanged(const float *, size_t) {    
}

void PlayerStateAdapter::OnGaplessPlayback() {
    QModelIndex index;
    if (next_playindex_.TryDequeue(index)) {
        gaplessPlayback(index);
    }
}

std::optional<AlignPtr<FileStream>> PlayerStateAdapter::GetNextGaplessStream() {
    AlignPtr<FileStream> stream;
    if (next_streams_.TryDequeue(stream)) {
        return std::move(stream);
    }
    return std::nullopt;
}