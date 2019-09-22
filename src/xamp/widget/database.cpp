#include "database.h"

Database::Database() {

}

int32_t Database::addOrUpdateMusic(const xamp::base::Metadata& medata, int32_t playlist_id) {
	return 0;
}

int32_t Database::addOrUpdateArtist(const QString& artist) {
	return 0;
}

int32_t Database::addOrUpdateAlbum(const QString& album, int32_t artist_id) {
	return 0;
}

int32_t Database::addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) {
	return 0;
}