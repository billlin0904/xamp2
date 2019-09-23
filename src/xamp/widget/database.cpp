#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include <vector>

#include "database.h"

#define ENSURE_EXEC(query) \
		if (!query.exec()) {\
			qDebug() << query.lastError().text();\
			throw std::invalid_argument("Sql execute fail");\
		}

#define ENSURE_EXEC_SQL(query, sql) \
		if (!query.exec(sql)) {\
			qDebug() << query.lastError().text();\
			throw std::invalid_argument("Sql execute fail");\
		}

Database::Database() {

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
	db_ = QSqlDatabase::addDatabase("QSQLITE");
	db_.setDatabaseName(file_name);
	if (!db_.open()) {
		auto message = "Database open fail! " + db_.lastError().text().toStdString();
		throw std::invalid_argument(message.c_str());
		return;
	}
	(void)db_.exec("PRAGMA synchronous = OFF");
	(void)db_.exec("PRAGMA auto_vacuum = FULL");
	(void)db_.exec("PRAGMA foreign_keys = ON");
	(void)db_.exec("PRAGMA journal_mode = MEMORY");
	(void)db_.exec("PRAGMA cache_size = 100000");
	(void)db_.exec("PRAGMA temp_store = MEMORY");
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