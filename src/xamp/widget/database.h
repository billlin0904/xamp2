//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <base/logger.h>
#include <base/exception.h>
#include <base/metadata.h>

#include "playlistentity.h"

class SqlException : public xamp::base::Exception {
public:
	explicit SqlException(QSqlError error);

	const char* what() const override;
};

#define IF_FAILED_SHOW_TOAST(expr) \
	try {\
		expr;\
	}\
	catch (const xamp::base::Exception& e) {\
		XAMP_LOG_DEBUG(e.what());\
	}

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

	void updateAlbumCover(int32_t album_id, const QString& album, const QString& cover_id);

	void addTablePlaylist(int32_t tableId, int32_t playlist_id);

	int32_t addOrUpdateMusic(const xamp::base::Metadata& medata, int32_t playlist_id);

	int32_t addOrUpdateArtist(const QString& artist);

	int32_t addOrUpdateAlbum(const QString& album, int32_t artist_id);

	void addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id);

	QString getAlbumCoverId(int32_t album_id) const;

	template <typename Function>
	void forEachPlaylistMusic(int32_t playlist_id, Function &&fun) {
		QSqlQuery query;

		query.prepare(R"(
		SELECT
			albumMusic.albumId,
			albumMusic.artistId,
			albums.album,
			albums.coverId,
			artists.artist,
			musics.* 
		FROM
			playlistMusics
			JOIN playlist ON playlist.playlistId = playlistMusics.playlistId
			JOIN albumMusic ON playlistMusics.musicId = albumMusic.musicId 
			JOIN musics ON playlistMusics.musicId = musics.musicId
			JOIN albums ON albumMusic.albumId = albums.albumId
			JOIN artists ON albumMusic.artistId = artists.artistId
		WHERE
			playlistMusics.playlistId = :playlist_id;
		)");

		query.bindValue(":playlist_id", playlist_id);

		if (!query.exec()) {
			XAMP_LOG_DEBUG("{}", query.lastError().text().toStdString());
		}

		while (query.next()) {
			PlayListEntity entity;
			entity.album_id = query.value("albumId").toInt();
			entity.artist_id = query.value("artistId").toInt();
			entity.music_id = query.value("musicId").toInt();
			entity.file_path = query.value("path").toString();
			entity.track = query.value("track").toInt();
			entity.title = query.value("title").toString();
			entity.file_name = query.value("fileName").toString();
			entity.album = query.value("album").toString();
			entity.artist = query.value("artist").toString();
			entity.file_ext = query.value("fileExt").toString();
			entity.duration = query.value("duration").toDouble();
			entity.bitrate = query.value("bitrate").toInt();
			entity.samplerate = query.value("samplerate").toInt();
			entity.cover_id = query.value("coverId").toString();
			fun(entity);
		}
	}

	void removePlaylistMusic(int32_t playlist_id, const QVector<int>& select_music_ids);

private:
	Database();

	void addAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const;

	void addMusicToPlaylist(int32_t music_id, int32_t playlist_id) const;

	void createTableIfNotExist();

	QSqlDatabase db_;
};

