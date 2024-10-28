#include <models/musicplayermodel.h>

MusicPlayerModel::MusicPlayerModel(const std::shared_ptr<IAudioPlayer>& player, QObject* parent)
	: QObject(parent)
	, player_(player) {
}

MusicPlayerModel::~MusicPlayerModel() {
}

void MusicPlayerModel::play(const QString& path) {

}

void MusicPlayerModel::stop() {
	player_->Stop(true, true, true);
}

const ScopedPtr<IAudioDeviceManager>& MusicPlayerModel::GetAudioDeviceManager() {
	return player_->GetAudioDeviceManager();
}

void MusicPlayerModel::seek(double seconds) {
	player_->Seek(seconds);
}

DsdModes MusicPlayerModel::dsdMode() const {
	return player_->GetDsdModes();
}

uint32_t MusicPlayerModel::volume() const {
	return player_->GetVolume();
}

PlayerState MusicPlayerModel::state() const {
	return player_->GetState();
}

void MusicPlayerModel::pause() {
	player_->Pause();
}

void MusicPlayerModel::resume() {
	player_->Resume();
}


