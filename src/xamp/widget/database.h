#pragma once

#include <QString>
#include <QSqlDatabase>

#include <base/metadata.h>

class Database {
public:
	static Database& Instance() {
		static Database instance;
		return instance;
	}

	void open(const QString& file_name);

	int32_t addOrUpdateMusic(const xamp::base::Metadata& medata, int32_t playlist_id);

	int32_t addOrUpdateArtist(const QString& artist);

	int32_t addOrUpdateAlbum(const QString& album, int32_t artist_id);

	int32_t addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id);
private:
	Database();

	void createTableIfNotExist();

	QSqlDatabase db_;
};

