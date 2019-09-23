//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QSqlDatabase>

#include <base/metadata.h>

class SqlException : public std::exception {
public:
	explicit SqlException(QSqlError error);

	const char* what() const override;
private:
	std::string message_;
};

class Database {
public:
	static const int32_t INVALID_DATABASE_ID = -1;

	static Database& Instance() {
		static Database instance;
		return instance;
	}

	~Database();

	void open(const QString& file_name);

	int32_t addTable(const QString& name, int32_t table_index);

	int32_t addPlaylist(const QString& name, int32_t playlistIndex);

	void addTablePlaylist(int32_t tableId, int32_t playlist_id);

	int32_t addOrUpdateMusic(const xamp::base::Metadata& medata, int32_t playlist_id);

	int32_t addOrUpdateArtist(const QString& artist);

	int32_t addOrUpdateAlbum(const QString& album, int32_t artist_id);

	void addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id);

	template <typename Function>
	void ForEachPlaylistMusic(int32_t playlist_id, Function &&fun) {
		/*
		SELECT * FROM
			playlistMusics
		JOIN
			playlist ON playlist.playlistId = playlistMusics.playlistId
		JOIN
			musics ON playlistMusics.musicId = musics.musicId
		WHERE
			playlistMusics.playlistId = 1;
		*/
	}
private:
	Database();

	void addAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const;

	void addMusicToPlaylist(int32_t music_id, int32_t playlist_id) const;

	void createTableIfNotExist();

	QSqlDatabase db_;
};

