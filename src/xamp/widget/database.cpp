#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>
#include <QDateTime>

#include <base/logger.h>
#include <widget/str_utilts.h>
#include <widget/database.h>

#define IfFailureThrow(query, sql) \
    do {\
    if (!query.exec(sql)) {\
    throw SqlException(query.lastError());\
    }\
    } while (false)

#define IfFailureThrow1(query) \
    do {\
    if (!query.exec()) {\
    throw SqlException(query.lastError());\
    }\
    } while (false)

SqlException::SqlException(QSqlError error)
	: Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR,
		error.text().toStdString()) {
	XAMP_LOG_DEBUG("SqlException: {}", error.text().toStdString());
}

const char* SqlException::what() const noexcept {
	return message_.c_str();
}

Database::Database() {
	db_ = QSqlDatabase::addDatabase(Q_TEXT("QSQLITE"));
}

Database::~Database() {
	db_.close();
}

void Database::flush() {
	
}

void Database::open(const QString& file_name) {
	db_.setDatabaseName(file_name);

	if (!db_.open()) {
		throw SqlException(db_.lastError());
	}

	dbname_ = file_name;
	(void)db_.exec(Q_TEXT("PRAGMA synchronous = OFF"));
	(void)db_.exec(Q_TEXT("PRAGMA auto_vacuum = OFF"));
	(void)db_.exec(Q_TEXT("PRAGMA foreign_keys = ON"));
	(void)db_.exec(Q_TEXT("PRAGMA journal_mode = MEMORY"));
	(void)db_.exec(Q_TEXT("PRAGMA cache_size = 40960"));
	(void)db_.exec(Q_TEXT("PRAGMA temp_store = MEMORY"));
	(void)db_.exec(Q_TEXT("PRAGMA mmap_size = 40960"));

	createTableIfNotExist();
}

void Database::createTableIfNotExist() {
	std::vector<QLatin1String> create_table_sql;

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS musics (
                       musicId integer PRIMARY KEY AUTOINCREMENT,
                       track integer,
                       title TEXT,
                       path TEXT NOT NULL,
                       parentPath TEXT NO NULL,
                       offset DOUBLE,
                       duration DOUBLE,
                       durationStr TEXT,
                       fileName TEXT,
                       fileExt TEXT,
                       bitrate integer,
                       samplerate integer,
					   rating integer,					   
                       dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                       album_replay_gain DOUBLE,
                       album_peak DOUBLE,
                       track_replay_gain DOUBLE,
                       track_peak DOUBLE,
					   genre TEXT,
					   comment TEXT,
					   year integer
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS playlist (
                       playlistId integer PRIMARY KEY AUTOINCREMENT,
                       playlistIndex integer,
                       name TEXT NOT NULL
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS tables (
                       tableId integer PRIMARY KEY AUTOINCREMENT,
                       tableIndex integer,
                       playlistId integer,
                       name TEXT NOT NULL
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS tablePlaylist (
                       playlistId integer,
                       tableId integer,
                       FOREIGN KEY(playlistId) REFERENCES playlist(playlistId),
                       FOREIGN KEY(tableId) REFERENCES tables(tableId)
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS albums (
                       albumId integer primary key autoincrement,
                       artistId integer,
                       album TEXT NOT NULL DEFAULT '',
                       coverId TEXT,
					   firstChar TEXT,
                       dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
					   isPodcast integer,
                       FOREIGN KEY(artistId) REFERENCES artists(artistId)
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS artists (
                       artistId integer primary key autoincrement,
                       artist TEXT NOT NULL DEFAULT '',
                       coverId TEXT,
					   firstChar TEXT,
                       dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS albumArtist (
                       albumArtistId integer primary key autoincrement,
                       artistId integer,
                       albumId integer,
                       FOREIGN KEY(artistId) REFERENCES artists(artistId),
                       FOREIGN KEY(albumId) REFERENCES albums(albumId)
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS albumMusic (
                       albumMusicId integer primary key autoincrement,
                       musicId integer,
                       artistId integer,
                       albumId integer,
                       FOREIGN KEY(musicId) REFERENCES musics(musicId),
                       FOREIGN KEY(artistId) REFERENCES artists(artistId),
                       FOREIGN KEY(albumId) REFERENCES albums(albumId)
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS playlistMusics (
					   playlistMusicsId integer primary key autoincrement,
                       playlistId integer,
                       musicId integer,
                       playing integer,
                       FOREIGN KEY(playlistId) REFERENCES playlist(playlistId),
                       FOREIGN KEY(musicId) REFERENCES musics(musicId)
                       )
                       )"));

	QSqlQuery query(db_);
	for (const auto& sql : create_table_sql) {
		IfFailureThrow(query, sql);
	}
}

void Database::clearNowPlaying(int32_t playlist_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("UPDATE playlistMusics SET playing = 0"));
	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	IfFailureThrow1(query);
}

void Database::clearNowPlaying(int32_t playlist_id, int32_t playlist_music_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("UPDATE playlistMusics SET playing = 0 WHERE (playlistId = :playlistId AND playlistMusicsId = :playlistMusicsId)"));
	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	query.bindValue(Q_TEXT(":playlistMusicsId"), playlist_music_id);
	IfFailureThrow1(query);
}

void Database::setNowPlaying(int32_t playlist_id, int32_t playlist_music_id) {
	clearNowPlaying(playlist_id);

	QSqlQuery query;
	query.prepare(Q_TEXT("UPDATE playlistMusics SET playing = 1 WHERE (playlistId = :playlistId AND playlistMusicsId = :playlistMusicsId)"));
	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	query.bindValue(Q_TEXT(":playlistMusicsId"), playlist_music_id);
	IfFailureThrow1(query);
}

void Database::removePlaylistMusics(int32_t music_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM playlistMusics WHERE musicId=:musicId"));
	query.bindValue(Q_TEXT(":musicId"), music_id);
	IfFailureThrow1(query);
}

void Database::removeAlbumMusicId(int32_t music_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM albumMusic WHERE musicId=:musicId"));
	query.bindValue(Q_TEXT(":musicId"), music_id);
	IfFailureThrow1(query);
}

void Database::removeAlbumArtistId(int32_t artist_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM albumArtist WHERE artistId=:artistId"));
	query.bindValue(Q_TEXT(":artistId"), artist_id);
	IfFailureThrow1(query);
}

void Database::removeMusic(int32_t music_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM musics WHERE musicId=:musicId"));
	query.bindValue(Q_TEXT(":musicId"), music_id);
	IfFailureThrow1(query);
}

void Database::removeMusic(QString const& file_path) {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT musicId FROM musics WHERE path = (:path)"));
	query.bindValue(Q_TEXT(":path"), file_path);

	query.exec();

	while (query.next()) {
		auto music_id = query.value(Q_TEXT("musicId")).toInt();
		removePlaylistMusic(1, QVector<int32_t>{ music_id });
		removeAlbumMusicId(music_id);
		removeAlbumArtistId(music_id);
		removeMusic(music_id);
		return;
	}
}

void Database::removeAlbumArtist(int32_t album_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM albumArtist WHERE albumId=:albumId"));
	query.bindValue(Q_TEXT(":albumId"), album_id);
	IfFailureThrow1(query);
}

void Database::forEachPlaylist(std::function<void(int32_t, int32_t, QString)>&& fun) {
    QSqlTableModel model(nullptr, db_);

    model.setTable(Q_TEXT("playlist"));
    model.setSort(1, Qt::AscendingOrder);
    model.select();

    for (auto i = 0; i < model.rowCount(); ++i) {
        auto record = model.record(i);
        fun(record.value(Q_TEXT("playlistId")).toInt(),
            record.value(Q_TEXT("playlistIndex")).toInt(),
            record.value(Q_TEXT("name")).toString());
    }
}

void Database::forEachTable(std::function<void(int32_t, int32_t, int32_t, QString)>&& fun) {
	QSqlTableModel model(nullptr, db_);

	model.setTable(Q_TEXT("tables"));
	model.setSort(1, Qt::AscendingOrder);
	model.select();

	for (auto i = 0; i < model.rowCount(); ++i) {
		auto record = model.record(i);
		fun(record.value(Q_TEXT("tableId")).toInt(),
			record.value(Q_TEXT("tableIndex")).toInt(),
			record.value(Q_TEXT("playlistId")).toInt(),
			record.value(Q_TEXT("name")).toString());
	}
}

void Database::forEachAlbumArtistMusic(int32_t album_id, int32_t artist_id, std::function<void(PlayListEntity const&)>&& fun) {
	QSqlQuery query(Q_TEXT(R"(
SELECT
    albumMusic.albumId,
    albumMusic.artistId,
    albums.album,
    albums.coverId,
    artists.artist,
    musics.*
FROM
    albumMusic
    LEFT JOIN albums ON albums.albumId = albumMusic.albumId
    LEFT JOIN artists ON artists.artistId = albumMusic.artistId
    LEFT JOIN musics ON musics.musicId = albumMusic.musicId
WHERE
    albums.albumId = ? AND albumMusic.artistId = ?
ORDER BY musics.path;)"));
	query.addBindValue(album_id);
	query.addBindValue(artist_id);

	if (!query.exec()) {
		XAMP_LOG_DEBUG("{}", query.lastError().text().toStdString());
	}

	while (query.next()) {
		PlayListEntity entity;
		entity.album_id = query.value(Q_TEXT("albumId")).toInt();
		entity.artist_id = query.value(Q_TEXT("artistId")).toInt();
		entity.music_id = query.value(Q_TEXT("musicId")).toInt();
		entity.file_path = query.value(Q_TEXT("path")).toString();
		entity.track = query.value(Q_TEXT("track")).toUInt();
		entity.title = query.value(Q_TEXT("title")).toString();
		entity.file_name = query.value(Q_TEXT("fileName")).toString();
		entity.album = query.value(Q_TEXT("album")).toString();
		entity.artist = query.value(Q_TEXT("artist")).toString();
		entity.file_ext = query.value(Q_TEXT("fileExt")).toString();
		entity.parent_path = query.value(Q_TEXT("parentPath")).toString();
		entity.duration = query.value(Q_TEXT("duration")).toDouble();
		entity.bitrate = query.value(Q_TEXT("bitrate")).toUInt();
		entity.samplerate = query.value(Q_TEXT("samplerate")).toUInt();
		entity.cover_id = query.value(Q_TEXT("coverId")).toString();
		entity.rating = query.value(Q_TEXT("rating")).toUInt();
		entity.album_replay_gain = query.value(Q_TEXT("album_replay_gain")).toDouble();
		entity.album_peak = query.value(Q_TEXT("album_peak")).toDouble();
		entity.track_replay_gain = query.value(Q_TEXT("track_replay_gain")).toDouble();
		entity.track_peak = query.value(Q_TEXT("track_peak")).toDouble();

		entity.genre = query.value(Q_TEXT("genre")).toString();
		entity.comment = query.value(Q_TEXT("comment")).toString();
		entity.year = query.value(Q_TEXT("year")).toUInt();
		fun(entity);
	}
}

void Database::forEachAlbumMusic(int32_t album_id, std::function<void(PlayListEntity const&)>&& fun) {
	QSqlQuery query(Q_TEXT(R"(
SELECT
    albumMusic.albumId,
    albumMusic.artistId,
    albums.album,
    albums.coverId,
    artists.artist,
    musics.*
FROM
    albumMusic
    LEFT JOIN albums ON albums.albumId = albumMusic.albumId
    LEFT JOIN artists ON artists.artistId = albumMusic.artistId
    LEFT JOIN musics ON musics.musicId = albumMusic.musicId
WHERE
    albums.albumId = ?
ORDER BY musics.path, musics.fileName;)"));
	query.addBindValue(album_id);

	if (!query.exec()) {
		XAMP_LOG_DEBUG("{}", query.lastError().text().toStdString());
	}

	while (query.next()) {
		PlayListEntity entity;
		entity.album_id = query.value(Q_TEXT("albumId")).toInt();
		entity.artist_id = query.value(Q_TEXT("artistId")).toInt();
		entity.music_id = query.value(Q_TEXT("musicId")).toInt();
		entity.file_path = query.value(Q_TEXT("path")).toString();
		entity.track = query.value(Q_TEXT("track")).toUInt();
		entity.title = query.value(Q_TEXT("title")).toString();
		entity.file_name = query.value(Q_TEXT("fileName")).toString();
		entity.album = query.value(Q_TEXT("album")).toString();
		entity.artist = query.value(Q_TEXT("artist")).toString();
		entity.file_ext = query.value(Q_TEXT("fileExt")).toString();
		entity.parent_path = query.value(Q_TEXT("parentPath")).toString();
		entity.duration = query.value(Q_TEXT("duration")).toDouble();
		entity.bitrate = query.value(Q_TEXT("bitrate")).toUInt();
		entity.samplerate = query.value(Q_TEXT("samplerate")).toUInt();
		entity.cover_id = query.value(Q_TEXT("coverId")).toString();
		entity.rating = query.value(Q_TEXT("rating")).toUInt();
		entity.album_replay_gain = query.value(Q_TEXT("album_replay_gain")).toDouble();
		entity.album_peak = query.value(Q_TEXT("album_peak")).toDouble();
		entity.track_replay_gain = query.value(Q_TEXT("track_replay_gain")).toDouble();
		entity.track_peak = query.value(Q_TEXT("track_peak")).toDouble();

		entity.genre = query.value(Q_TEXT("genre")).toString();
		entity.comment = query.value(Q_TEXT("comment")).toString();
		entity.year = query.value(Q_TEXT("year")).toUInt();
		fun(entity);
	}
}

void Database::removeAllArtist() {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM artists"));
	IfFailureThrow1(query);
}

void Database::removeArtistId(int32_t artist_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM artists WHERE artistId=:artistId"));
	query.bindValue(Q_TEXT(":artistId"), artist_id);
	IfFailureThrow1(query);
}

void Database::forEachAlbum(std::function<void(int32_t)>&& fun) {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT albumId FROM albums"));
	IfFailureThrow1(query);
	while (query.next()) {
		fun(query.value(Q_TEXT("albumId")).toInt());
	}
}

void Database::removeAlbum(int32_t album_id) {
	forEachAlbumMusic(album_id, [this](auto const& entity) {
        forEachPlaylist([&entity, this](auto playlistId, auto , auto) {
			removeMusic(playlistId, QVector<int32_t>{ entity.music_id });
        });
		removeAlbumMusicId(entity.music_id);
		removeMusic(entity.music_id);
		});

	removeAlbumArtist(album_id);

	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM albums WHERE albumId=:albumId"));
	query.bindValue(Q_TEXT(":albumId"), album_id);
	IfFailureThrow1(query);
}

int32_t Database::addTable(const QString& name, int32_t table_index, int32_t playlist_id) {
	QSqlTableModel model(nullptr, db_);

	model.setEditStrategy(QSqlTableModel::OnManualSubmit);
	model.setTable(Q_TEXT("tables"));
	model.select();

	if (!model.insertRows(0, 1)) {
		return kInvalidId;
	}

	model.setData(model.index(0, 0), QVariant());
	model.setData(model.index(0, 1), table_index);
	model.setData(model.index(0, 2), playlist_id);
	model.setData(model.index(0, 3), name);

	if (!model.submitAll()) {
		return kInvalidId;
	}

	model.database().commit();
	return model.query().lastInsertId().toInt();
}

int32_t Database::addPlaylist(const QString& name, int32_t playlist_index) {
	QSqlTableModel model(nullptr, db_);

	model.setEditStrategy(QSqlTableModel::OnManualSubmit);
	model.setTable(Q_TEXT("playlist"));
	model.select();

	if (!model.insertRows(0, 1)) {
		return kInvalidId;
	}

	model.setData(model.index(0, 0), QVariant());
	model.setData(model.index(0, 1), playlist_index);
	model.setData(model.index(0, 2), name);

	if (!model.submitAll()) {
		return kInvalidId;
	}

	model.database().commit();
	return model.query().lastInsertId().toInt();
}

void Database::setTableName(int32_t table_id, const QString& name) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE tables SET name = :name WHERE (tableId = :tableId)"));

	query.bindValue(Q_TEXT(":tableId"), table_id);
	query.bindValue(Q_TEXT(":name"), name);
	IfFailureThrow1(query);
}

void Database::setAlbumCover(int32_t album_id, const QString& album, const QString& cover_id) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE albums SET coverId = :coverId WHERE (albumId = :albumId) AND (album = :album)"));

	query.bindValue(Q_TEXT(":albumId"), album_id);
	query.bindValue(Q_TEXT(":album"), album);
	query.bindValue(Q_TEXT(":coverId"), cover_id);

	IfFailureThrow1(query);
}

std::optional<ArtistStats> Database::getArtistStats(int32_t artist_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    SELECT
        SUM(musics.duration) AS durations,
        (SELECT COUNT( * ) AS albums FROM albums WHERE albums.artistId = :artistId) AS albums,
		(SELECT COUNT( * ) AS tracks FROM albumMusic WHERE albumMusic.artistId = :artistId) AS tracks
    FROM
	    albumMusic
	JOIN albums ON albums.artistId = albumMusic.artistId 
    JOIN musics ON musics.musicId = albumMusic.musicId 
    WHERE
	    albums.artistId = :artistId;)"));

	query.bindValue(Q_TEXT(":artistId"), artist_id);

	IfFailureThrow1(query);

	while (query.next()) {
		ArtistStats stats;
		stats.albums = query.value(Q_TEXT("albums")).toInt();
		stats.tracks = query.value(Q_TEXT("tracks")).toInt();
		stats.durations = query.value(Q_TEXT("durations")).toDouble();
		return stats;
	}

	return std::nullopt;
}

std::optional<AlbumStats> Database::getAlbumStats(int32_t album_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    SELECT
        SUM(musics.duration) AS durations,
        (SELECT COUNT( * ) AS tracks FROM albumMusic WHERE albumMusic.albumId = :albumId) AS tracks
    FROM
	    albumMusic
	JOIN albums ON albums.albumId = albumMusic.albumId 
    JOIN musics ON musics.musicId = albumMusic.musicId 
    WHERE
	    albums.albumId = :albumId;)"));

	query.bindValue(Q_TEXT(":albumId"), album_id);

	IfFailureThrow1(query);

	while (query.next()) {
		AlbumStats stats;
		stats.tracks = query.value(Q_TEXT("tracks")).toInt();
		stats.durations = query.value(Q_TEXT("durations")).toDouble();
		return stats;
	}

	return std::nullopt;
}

bool Database::isPlaylistExist(int32_t playlist_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT playlistId FROM playlist WHERE playlistId = (:playlistId)"));
	query.bindValue(Q_TEXT(":playlistId"), playlist_id);

	IfFailureThrow1(query);
	return query.next();
}

int32_t Database::findTablePlaylistId(int32_t table_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT playlistId FROM tablePlaylist WHERE tableId = (:tableId)"));
	query.bindValue(Q_TEXT(":tableId"), table_id);

	IfFailureThrow1(query);
	while (query.next()) {
		return query.value(Q_TEXT("playlistId")).toInt();
	}
	return kInvalidId;
}

void Database::addTablePlaylist(int32_t tableId, int32_t playlist_id) {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    INSERT INTO tablePlaylist (playlistId, tableId) VALUES (:playlistId, :tableId)
    )"));

	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	query.bindValue(Q_TEXT(":tableId"), tableId);

	IfFailureThrow1(query);
}

QString Database::getArtistCoverId(int32_t artist_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT coverId FROM artists WHERE artistId = (:artistId)"));
	query.bindValue(Q_TEXT(":artistId"), artist_id);

	IfFailureThrow1(query);

	const auto index = query.record().indexOf(Q_TEXT("coverId"));
	if (query.next()) {
		return query.value(index).toString();
	}
	return Qt::EmptyString;
}

QString Database::getAlbumCoverId(int32_t album_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT coverId FROM albums WHERE albumId = (:albumId)"));
	query.bindValue(Q_TEXT(":albumId"), album_id);

	IfFailureThrow1(query);

	const auto index = query.record().indexOf(Q_TEXT("coverId"));
	if (query.next()) {
		return query.value(index).toString();
	}
	return Qt::EmptyString;
}

QString Database::getAlbumCoverId(const QString& album) const {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT coverId FROM albums WHERE album = (:album)"));
	query.bindValue(Q_TEXT(":album"), album);

	IfFailureThrow1(query);

	const auto index = query.record().indexOf(Q_TEXT("coverId"));
	if (query.next()) {
		return query.value(index).toString();
	}
	return Qt::EmptyString;
}

int32_t Database::addOrUpdateMusic(const Metadata& metadata, int32_t playlist_id) {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    INSERT OR REPLACE INTO musics
    (musicId, title, track, path, fileExt, fileName, duration, durationStr, parentPath, bitrate, samplerate, offset, dateTime, album_replay_gain, track_replay_gain, album_peak, track_peak, genre, comment, year)
    VALUES ((SELECT musicId FROM musics WHERE path = :path and offset = :offset), :title, :track, :path, :fileExt, :fileName, :duration, :durationStr, :parentPath, :bitrate, :samplerate, :offset, :dateTime, :album_replay_gain, :track_replay_gain, :album_peak, :track_peak, :genre, :comment, :year)
    )")
	);

	query.bindValue(Q_TEXT(":title"), QString::fromStdWString(metadata.title));
	query.bindValue(Q_TEXT(":track"), metadata.track);
	query.bindValue(Q_TEXT(":path"), QString::fromStdWString(metadata.file_path));
	query.bindValue(Q_TEXT(":fileExt"), QString::fromStdWString(metadata.file_ext));
	query.bindValue(Q_TEXT(":fileName"), QString::fromStdWString(metadata.file_name));
	query.bindValue(Q_TEXT(":parentPath"), QString::fromStdWString(metadata.parent_path));
	query.bindValue(Q_TEXT(":duration"), metadata.duration);
	query.bindValue(Q_TEXT(":durationStr"), msToString(metadata.duration));
	query.bindValue(Q_TEXT(":bitrate"), metadata.bitrate);
	query.bindValue(Q_TEXT(":samplerate"), metadata.samplerate);
	query.bindValue(Q_TEXT(":offset"), metadata.offset);

	if (metadata.replay_gain) {
		query.bindValue(Q_TEXT(":album_replay_gain"), metadata.replay_gain.value().album_gain);
		query.bindValue(Q_TEXT(":track_replay_gain"), metadata.replay_gain.value().track_gain);
		query.bindValue(Q_TEXT(":album_peak"), metadata.replay_gain.value().album_peak);
		query.bindValue(Q_TEXT(":track_peak"), metadata.replay_gain.value().track_peak);
	}
	else {
		query.bindValue(Q_TEXT(":album_replay_gain"), 0);
		query.bindValue(Q_TEXT(":track_replay_gain"), 0);
		query.bindValue(Q_TEXT(":album_peak"), 0);
		query.bindValue(Q_TEXT(":track_peak"), 0);
	}

	if (metadata.last_write_time == 0) {
		query.bindValue(Q_TEXT(":dateTime"), QDateTime::currentSecsSinceEpoch());
	}
	else {
		query.bindValue(Q_TEXT(":dateTime"), metadata.last_write_time);
	}

	query.bindValue(Q_TEXT(":genre"), QString::fromStdWString(metadata.genre));
	query.bindValue(Q_TEXT(":comment"), QString::fromStdWString(metadata.comment));
	query.bindValue(Q_TEXT(":year"), metadata.year);

	db_.transaction();

	if (!query.exec()) {
		qDebug() << query.lastError().text();
		return kInvalidId;
	}

	const auto music_id = query.lastInsertId().toInt();

	if (playlist_id != -1) {
		addMusicToPlaylist(music_id, playlist_id);
	}

	db_.commit();
	return music_id;
}

void Database::updateMusicFilePath(int32_t music_id, const QString& file_path) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE musics SET path = :path WHERE (musicId = :musicId)"));

	query.bindValue(Q_TEXT(":musicId"), music_id);
	query.bindValue(Q_TEXT(":path"), file_path);

	query.exec();
}

void Database::updateMusicRating(int32_t music_id, int32_t rating) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE musics SET rating = :rating WHERE (musicId = :musicId)"));

	query.bindValue(Q_TEXT(":musicId"), music_id);
	query.bindValue(Q_TEXT(":rating"), rating);

	IfFailureThrow1(query);
}

void Database::updateReplayGain(int music_id,
	double album_rg_gain,
	double album_peak,
	double track_rg_gain,
	double track_peak) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE musics SET album_replay_gain = :album_replay_gain, album_peak = :album_peak, track_replay_gain = :track_replay_gain, track_peak = :track_peak WHERE (musicId = :musicId)"));

	query.bindValue(Q_TEXT(":musicId"), music_id);
	query.bindValue(Q_TEXT(":album_replay_gain"), album_rg_gain);
	query.bindValue(Q_TEXT(":album_peak"), album_peak);
	query.bindValue(Q_TEXT(":track_replay_gain"), track_rg_gain);
	query.bindValue(Q_TEXT(":track_peak"), track_peak);

	IfFailureThrow1(query);
}

void Database::addMusicToPlaylist(int32_t music_id, int32_t playlist_id) const {
	QSqlQuery query;

	const auto querystr = Q_STR("INSERT INTO playlistMusics (playlistMusicsId, playlistId, musicId) VALUES (NULL, %1, %2)")
		.arg(playlist_id)
		.arg(music_id);

	query.prepare(querystr);
	IfFailureThrow1(query);
}


void Database::addMusicToPlaylist(const Vector<int32_t>& music_id, int32_t playlist_id) const {
	QSqlQuery query;

	QStringList strings;
	strings.reserve(music_id.size());

	for (const auto id : music_id) {
		strings << Q_TEXT("(") + Q_TEXT("NULL, ") + QString::number(playlist_id) + Q_TEXT(", ") + QString::number(id) + Q_TEXT(")");
	}

	const auto querystr = Q_TEXT("INSERT INTO playlistMusics (playlistMusicsId, playlistId, musicId) VALUES ")
	+ strings.join(Q_TEXT(","));
	query.prepare(querystr);
	IfFailureThrow1(query);
}

void Database::updateArtistCoverId(int32_t artist_id, const QString& coverId) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE artists SET coverId = :coverId WHERE (artistId = :artistId)"));

	query.bindValue(Q_TEXT(":artistId"), artist_id);
	query.bindValue(Q_TEXT(":coverId"), coverId);

	IfFailureThrow1(query);
}

int32_t Database::addOrUpdateArtist(const QString& artist) {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    INSERT OR REPLACE INTO artists (artistId, artist, firstChar)
    VALUES ((SELECT artistId FROM artists WHERE artist = :artist), :artist, :firstChar)
    )"));

	auto firstChar = artist.left(1);
	query.bindValue(Q_TEXT(":artist"), artist);
	query.bindValue(Q_TEXT(":firstChar"), firstChar.toUpper());

	IfFailureThrow1(query);

	const auto artist_id = query.lastInsertId().toInt();
	return artist_id;
}

int32_t Database::addOrUpdateAlbum(const QString& album, int32_t artist_id, int64_t album_time, bool is_podcast) {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    INSERT OR REPLACE INTO albums (albumId, album, artistId, firstChar, coverId, isPodcast, dateTime)
    VALUES ((SELECT albumId FROM albums WHERE album = :album), :album, :artistId, :firstChar, :coverId, :isPodcast, :dateTime)
    )"));

	const auto first_char = album.left(1);

	query.bindValue(Q_TEXT(":album"), album);
	query.bindValue(Q_TEXT(":artistId"), artist_id);
	query.bindValue(Q_TEXT(":firstChar"), first_char.toUpper());
	query.bindValue(Q_TEXT(":coverId"), getAlbumCoverId(album));
	query.bindValue(Q_TEXT(":isPodcast"), is_podcast ? 1 : 0);
	query.bindValue(Q_TEXT(":dateTime"), album_time);

	IfFailureThrow1(query);

	const auto album_id = query.lastInsertId().toInt();
	return album_id;
}

void Database::addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
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
    )"));

	query.bindValue(Q_TEXT(":album"), album_id);
	query.bindValue(Q_TEXT(":artist"), artist_id);
	query.bindValue(Q_TEXT(":musicId"), music_id);

	db_.transaction();

	IfFailureThrow1(query);

	if (!query.next()) {
		addAlbumMusic(album_id, artist_id, music_id);
		db_.commit();
	}
}

void Database::addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    INSERT OR REPLACE INTO albumArtist (albumArtistId, albumId, artistId)
    VALUES ((SELECT albumArtistId from albumArtist where albumId = :albumId AND artistId = :artistId), :albumId, :artistId)
    )"));

	query.bindValue(Q_TEXT(":albumId"), album_id);
	query.bindValue(Q_TEXT(":artistId"), artist_id);

	IfFailureThrow1(query);
}

void Database::addAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    INSERT OR REPLACE INTO albumMusic (albumMusicId, albumId, artistId, musicId)
    VALUES ((SELECT albumMusicId from albumMusic where albumId = :albumId AND artistId = :artistId AND musicId = :musicId), :albumId, :artistId, :musicId)
    )"));

	query.bindValue(Q_TEXT(":albumId"), album_id);
	query.bindValue(Q_TEXT(":artistId"), artist_id);
	query.bindValue(Q_TEXT(":musicId"), music_id);

	IfFailureThrow1(query);

	addOrUpdateAlbumArtist(album_id, artist_id);
}

void Database::removePlaylistAllMusic(int32_t playlist_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM playlistMusics WHERE playlistId=:playlistId"));
	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	IfFailureThrow1(query);
}

void Database::removeMusic(int32_t playlist_id, const QVector<int32_t>& select_music_ids) {
	QSqlQuery query;

	QString str = Q_TEXT("DELETE FROM playlistMusics WHERE playlistId=:playlistId AND musicId in (%0)");

	QStringList list;
	for (auto id : select_music_ids) {
		list << QString::number(id);
	}

	auto q = str.arg(list.join(Q_TEXT(",")));
	query.prepare(q);

	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	IfFailureThrow1(query);
}

void Database::removePlaylistMusic(int32_t playlist_id, const QVector<int32_t>& select_music_ids) {
	QSqlQuery query;

	QString str = Q_TEXT("DELETE FROM playlistMusics WHERE playlistId=:playlistId AND playlistMusicsId in (%0)");

	QStringList list;
	for (auto id : select_music_ids) {
		list << QString::number(id);
	}

	auto q = str.arg(list.join(Q_TEXT(",")));
	query.prepare(q);

	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	IfFailureThrow1(query);
}
