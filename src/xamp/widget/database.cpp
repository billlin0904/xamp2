#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

#include "time_utilts.h"
#include "database.h"

#define ENSURE_EXEC(query) \
		if (!query.exec()) {\
			throw SqlException(query.lastError());\
		}

#define ENSURE_EXEC_SQL(query, sql) \
		if (!query.exec(sql)) {\
			throw SqlException(query.lastError());\
		}

SqlException::SqlException(QSqlError error)
	: message_(error.text().toStdString()) {
}

const char* SqlException::what() const {
	return message_.c_str();
}

Database::Database() {
	db_ = QSqlDatabase::addDatabase("QSQLITE");
}

Database::~Database() {	
	db_.close();
}

void Database::createTableIfNotExist() {
	std::vector<QString> create_table_sql;

	create_table_sql.push_back(
		R"(
		CREATE TABLE IF NOT EXISTS musics (
			musicId integer PRIMARY KEY AUTOINCREMENT,
			track integer,
			title TEXT,
			path TEXT NOT NULL,
			parentPath TEXT NO NULL,
			lrc TEXT,
			offset DOUBLE,
			duration DOUBLE,
			durationStr TEXT,
            fileName TEXT,
			fileExt TEXT,
            bitrate integer,
			samplerate integer,
			dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP
		)
	)");

	create_table_sql.push_back(
		R"(
		CREATE TABLE IF NOT EXISTS playlist (
			playlistId integer PRIMARY KEY AUTOINCREMENT,
			playlistIndex integer,
			name TEXT NOT NULL,
			unique (name)
		)
	)");

	create_table_sql.push_back(
		R"(
		CREATE TABLE IF NOT EXISTS tables (
			tableId integer PRIMARY KEY AUTOINCREMENT,            
			tableIndex integer,
            playlistId integer,
			name TEXT NOT NULL,
			unique (name)
		)
	)");

	create_table_sql.push_back(
		R"(
		CREATE TABLE IF NOT EXISTS tablePlaylist (
			playlistId integer,
			tableId integer,
			FOREIGN KEY(playlistId) REFERENCES playlist(playlistId),
			FOREIGN KEY(tableId) REFERENCES tables(tableId)
		)
	)");

	create_table_sql.push_back(
		R"(
		CREATE TABLE IF NOT EXISTS albums (
			albumId integer primary key autoincrement,
			artistId integer,
			album TEXT NOT NULL DEFAULT '',
            coverId TEXT,
			dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
			FOREIGN KEY(artistId) REFERENCES artists(artistId)
		)
	)");

	create_table_sql.push_back(
		R"(
		CREATE TABLE IF NOT EXISTS artists (
			artistId integer primary key autoincrement,
			artist TEXT NOT NULL DEFAULT '',
            coverId TEXT,
			dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP
		)
	)");

	create_table_sql.push_back(
		R"(
		CREATE TABLE IF NOT EXISTS albumMusic (
			albumMusicId integer primary key autoincrement,
			musicId integer,
			artistId integer,
			albumId integer,
			FOREIGN KEY(musicId) REFERENCES musics(musicId),
			FOREIGN KEY(artistId) REFERENCES artists(artistId),
			FOREIGN KEY(albumId) REFERENCES albums(albumId)
		)
	)");

	create_table_sql.push_back(
		R"(
		CREATE TABLE IF NOT EXISTS playlistMusics (
			playlistId integer,
			musicId integer,
			FOREIGN KEY(playlistId) REFERENCES playlist(playlistId),
			FOREIGN KEY(musicId) REFERENCES musics(musicId)
		)
	)");

	QSqlQuery query(db_);
	for (const auto& sql : create_table_sql) {
		ENSURE_EXEC_SQL(query, sql);
	}
}

void Database::open(const QString& file_name) {	
	db_.setDatabaseName(file_name);

	if (!db_.open()) {
		throw SqlException(db_.lastError());
	}

	(void)db_.exec("PRAGMA synchronous = OFF");
	(void)db_.exec("PRAGMA auto_vacuum = FULL");
	(void)db_.exec("PRAGMA foreign_keys = ON");
	(void)db_.exec("PRAGMA journal_mode = MEMORY");
	(void)db_.exec("PRAGMA cache_size = 100000");
	(void)db_.exec("PRAGMA temp_store = MEMORY");

	createTableIfNotExist();
}

int32_t Database::addTable(const QString& name, int32_t table_index) {
	QSqlTableModel model(nullptr, db_);

	model.setEditStrategy(QSqlTableModel::OnManualSubmit);
	model.setTable("tables");
	model.select();

	if (!model.insertRows(0, 1)) {
		return INVALID_DATABASE_ID;
	}

	model.setData(model.index(0, 0), QVariant());
	model.setData(model.index(0, 1), table_index);
	model.setData(model.index(0, 2), 0);
	model.setData(model.index(0, 3), name);

	if (!model.submitAll()) {
		return INVALID_DATABASE_ID;
	}

	model.database().commit();
	return model.query().lastInsertId().toInt();
}

int32_t Database::addPlaylist(const QString& name, int32_t playlist_index) {
	QSqlTableModel model(nullptr, db_);

	model.setEditStrategy(QSqlTableModel::OnManualSubmit);
	model.setTable("playlist");
	model.select();

	if (!model.insertRows(0, 1)) {
		return INVALID_DATABASE_ID;
	}

	model.setData(model.index(0, 0), QVariant());
	model.setData(model.index(0, 1), playlist_index);
	model.setData(model.index(0, 2), name);

	if (!model.submitAll()) {
		return INVALID_DATABASE_ID;
	}

	model.database().commit();
	return model.query().lastInsertId().toInt();
}

void Database::updateAlbumCover(int32_t album_id, const QString& album, const QString& cover_id) {
	QSqlQuery query(db_);

	query.prepare("UPDATE albums SET coverId = :coverId WHERE (albumId = :albumId) OR (album = :album)");

	query.bindValue(":albumId", album_id);
	query.bindValue(":album", album);
	query.bindValue(":coverId", cover_id);

	ENSURE_EXEC(query);
}

void Database::addTablePlaylist(int32_t tableId, int32_t playlist_id) {
	QSqlQuery query;

	query.prepare(R"(
        INSERT INTO tablePlaylist (playlistId, tableId) VALUES (:playlistId, :tableId)
    )");

	query.bindValue(":playlistId", playlist_id);
	query.bindValue(":tableId", tableId);

	ENSURE_EXEC(query);
}

QString Database::getAlbumCoverId(int32_t album_id) const {
	QSqlQuery query;

	query.prepare("SELECT coverId FROM albums WHERE albumId = (:albumId)");
	query.bindValue(":albumId", album_id);

	ENSURE_EXEC(query);

	const auto album_cover_tag_id_index = query.record().indexOf("coverId");
	if (query.next()) {
		return query.value(album_cover_tag_id_index).toString();
	}
	return "";
}

int32_t Database::addOrUpdateMusic(const xamp::base::Metadata& metadata, int32_t playlist_id) {
	QSqlQuery query;

	query.prepare(
		R"(
		INSERT OR REPLACE INTO musics
		       (musicId, title, track, path, fileExt, fileName, duration, durationStr, parentPath, bitrate, samplerate, offset)
		VALUES ((SELECT musicId FROM musics WHERE path = :path and offset = :offset), :title, :track, :path, :fileExt, :fileName, :duration, :durationStr, :parentPath, :bitrate, :samplerate, :offset)
		)");

	auto album = QString::fromStdWString(metadata.album);
	auto artist = QString::fromStdWString(metadata.artist);

	query.bindValue(":title", QString::fromStdWString(metadata.title));
	query.bindValue(":track", metadata.track);
	query.bindValue(":path", QString::fromStdWString(metadata.file_path));
	query.bindValue(":fileExt", QString::fromStdWString(metadata.file_ext));
	query.bindValue(":fileName", QString::fromStdWString(metadata.file_name));
	query.bindValue(":parentPath", QString::fromStdWString(metadata.parent_path));
	query.bindValue(":duration", metadata.duration);
	query.bindValue(":durationStr", Time::msToString(metadata.duration));
	query.bindValue(":bitrate", metadata.bitrate);
	query.bindValue(":samplerate", metadata.samplerate);
	query.bindValue(":offset", metadata.offset);

	db_.transaction();

	if (!query.exec()) {
		qDebug() << query.lastError().text();
		return INVALID_DATABASE_ID;
	}

	auto music_id = query.lastInsertId().toInt();
	//addMusicToPlaylist(music_id, playlist_id);

	db_.commit();
	return music_id;
}

void Database::addMusicToPlaylist(int32_t music_id, int32_t playlist_id) const {
	QSqlQuery query;

	query.prepare(R"(
        INSERT INTO playlistMusics (playlistId, musicId) VALUES (:playlistId, :musicId)
    )");

	query.bindValue(":playlistId", playlist_id);
	query.bindValue(":musicId", music_id);
	ENSURE_EXEC(query);
}

int32_t Database::addOrUpdateArtist(const QString& artist) {
	QSqlQuery query;

	query.prepare(
		R"(
		INSERT OR REPLACE INTO artists (artistId, artist)
		VALUES ((SELECT artistId FROM artists WHERE artist = :artist), :artist)
		)");

	query.bindValue(":artist", artist);

	ENSURE_EXEC(query);

	const auto artist_id = query.lastInsertId().toInt();
	return artist_id;
}

int32_t Database::addOrUpdateAlbum(const QString& album, int32_t artist_id) {
	QSqlQuery query;

	query.prepare(
		R"(
		INSERT OR REPLACE INTO albums (albumId, album, artistId)
		VALUES ((SELECT albumId FROM albums WHERE album = :album), :album, :artistId)
		)");
	
	query.bindValue(":album", album);
	query.bindValue(":artistId", artist_id);

	ENSURE_EXEC(query);

	const auto album_id = query.lastInsertId().toInt();
	return album_id;
}

void Database::addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) {
	QSqlQuery query;

	query.prepare(R"(
	SELECT
		albumMusicId
	FROM
		albumMusic
	JOIN
		albums ON albums.albumId = albumMusic.albumId
	JOIN
		artists ON artists.artistId = albumMusic.artistId
	JOIN 
		musics ON musics.musicId = albumMusic.musicId
	WHERE
		albums.albumId = :albumId 
	AND 
		artists.artistId = :artistId
	AND 
		musics.musicId = :musicId;
	)");

	query.bindValue(":album", album_id);
	query.bindValue(":artist", artist_id);
	query.bindValue(":musicId", music_id);

	db_.transaction();

	ENSURE_EXEC(query);

	if (!query.next()) {
		addAlbumMusic(album_id, artist_id, music_id);
		db_.commit();
	}
}

void Database::addAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const {
	QSqlQuery query;

	query.prepare(R"(
		INSERT OR REPLACE INTO albumMusic (albumMusicId, albumId, artistId, musicId) 
        VALUES ((SELECT albumMusicId from albumMusic where albumId = :albumId AND artistId = :artistId AND musicId = :musicId), :albumId, :artistId, :musicId)
	)");

	query.bindValue(":albumId", album_id);
	query.bindValue(":artistId", artist_id);
	query.bindValue(":musicId", music_id);

	ENSURE_EXEC(query);
}