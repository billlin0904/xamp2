#include <QSqlTableModel>
#include <QSqlRecord>
#include <QDebug>
#include <QDate>
#include <base/time.h>
#include <widget/time_utilts.h>
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
}

const char* SqlException::what() const noexcept {
	return message_.c_str();
}

Database::Database() {
	db_ = QSqlDatabase::addDatabase(Q_UTF8("QSQLITE"));
}

Database::~Database() {
	db_.close();
}

void Database::createTableIfNotExist() {
	std::vector<QLatin1String> create_table_sql;

	create_table_sql.push_back(
		Q_UTF8(R"(
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
				       fingerprint TEXT,
                       bitrate integer,
                       samplerate integer,
					   rating integer,
                       dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                       lufs DOUBLE,
					   true_peak DOUBLE
                       )
                       )"));

	create_table_sql.push_back(
		Q_UTF8(R"(
                       CREATE TABLE IF NOT EXISTS playlist (
                       playlistId integer PRIMARY KEY AUTOINCREMENT,
                       playlistIndex integer,
                       name TEXT NOT NULL
                       )
                       )"));

	create_table_sql.push_back(
		Q_UTF8(R"(
                       CREATE TABLE IF NOT EXISTS tables (
                       tableId integer PRIMARY KEY AUTOINCREMENT,
                       tableIndex integer,
                       playlistId integer,
                       name TEXT NOT NULL
                       )
                       )"));

	create_table_sql.push_back(
		Q_UTF8(R"(
                       CREATE TABLE IF NOT EXISTS tablePlaylist (
                       playlistId integer,
                       tableId integer,
                       FOREIGN KEY(playlistId) REFERENCES playlist(playlistId),
                       FOREIGN KEY(tableId) REFERENCES tables(tableId)
                       )
                       )"));

	create_table_sql.push_back(
		Q_UTF8(R"(
                       CREATE TABLE IF NOT EXISTS albums (
                       albumId integer primary key autoincrement,
                       artistId integer,
                       album TEXT NOT NULL DEFAULT '',
                       coverId TEXT,
					   firstChar TEXT,
                       dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                       FOREIGN KEY(artistId) REFERENCES artists(artistId)
                       )
                       )"));

	create_table_sql.push_back(
		Q_UTF8(R"(
                       CREATE TABLE IF NOT EXISTS artists (
                       artistId integer primary key autoincrement,
                       artist TEXT NOT NULL DEFAULT '',
                       coverId TEXT,
                       discogsArtistId TEXT,
                       mbid TEXT,
					   firstChar TEXT,
                       dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                       )
                       )"));

	create_table_sql.push_back(
		Q_UTF8(R"(
                       CREATE TABLE IF NOT EXISTS albumArtist (
                       albumArtistId integer primary key autoincrement,
                       artistId integer,
                       albumId integer,
                       FOREIGN KEY(artistId) REFERENCES artists(artistId),
                       FOREIGN KEY(albumId) REFERENCES albums(albumId)
                       )
                       )"));

	create_table_sql.push_back(
		Q_UTF8(R"(
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
		Q_UTF8(R"(
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

void Database::open(const QString& file_name) {
	db_.setDatabaseName(file_name);

	if (!db_.open()) {
		throw SqlException(db_.lastError());
	}

	(void)db_.exec(Q_UTF8("PRAGMA synchronous = OFF"));
	(void)db_.exec(Q_UTF8("PRAGMA auto_vacuum = OFF"));
	(void)db_.exec(Q_UTF8("PRAGMA foreign_keys = ON"));
	(void)db_.exec(Q_UTF8("PRAGMA journal_mode = MEMORY"));
	(void)db_.exec(Q_UTF8("PRAGMA cache_size = 40960"));
	(void)db_.exec(Q_UTF8("PRAGMA temp_store = MEMORY"));
	(void)db_.exec(Q_UTF8("PRAGMA mmap_size = 40960"));

	createTableIfNotExist();
}

void Database::flush() {
}

void Database::clearNowPlaying(int32_t playlist_id) {
    QSqlQuery query;
    query.prepare(Q_UTF8("UPDATE playlistMusics SET playing = 0"));
    query.bindValue(Q_UTF8(":playlistId"), playlist_id);
    IfFailureThrow1(query);
}

void Database::clearNowPlaying(int32_t playlist_id, int32_t playlist_music_id) {
	QSqlQuery query;
	query.prepare(Q_UTF8("UPDATE playlistMusics SET playing = 0 WHERE (playlistId = :playlistId AND playlistMusicsId = :playlistMusicsId)"));
	query.bindValue(Q_UTF8(":playlistId"), playlist_id);
	query.bindValue(Q_UTF8(":playlistMusicsId"), playlist_music_id);
	IfFailureThrow1(query);
}

void Database::setNowPlaying(int32_t playlist_id, int32_t playlist_music_id) {
    clearNowPlaying(playlist_id);

    QSqlQuery query;
    query.prepare(Q_UTF8("UPDATE playlistMusics SET playing = 1 WHERE (playlistId = :playlistId AND playlistMusicsId = :playlistMusicsId)"));
    query.bindValue(Q_UTF8(":playlistId"), playlist_id);
    query.bindValue(Q_UTF8(":playlistMusicsId"), playlist_music_id);
    IfFailureThrow1(query);
}

void Database::removePlaylistMusics(int32_t music_id) {
    QSqlQuery query;
    query.prepare(Q_UTF8("DELETE FROM playlistMusics WHERE musicId=:musicId"));
    query.bindValue(Q_UTF8(":musicId"), music_id);
    IfFailureThrow1(query);
}

void Database::removeAlbumMusicId(int32_t music_id) {
    QSqlQuery query;
    query.prepare(Q_UTF8("DELETE FROM albumMusic WHERE musicId=:musicId"));
    query.bindValue(Q_UTF8(":musicId"), music_id);
    IfFailureThrow1(query);
}

void Database::removeAlbumArtistId(int32_t artist_id) {
    QSqlQuery query;
    query.prepare(Q_UTF8("DELETE FROM albumArtist WHERE artistId=:artistId"));
    query.bindValue(Q_UTF8(":artistId"), artist_id);
    IfFailureThrow1(query);
}

void Database::removeMusic(int32_t music_id) {
    QSqlQuery query;
    query.prepare(Q_UTF8("DELETE FROM musics WHERE musicId=:musicId"));
    query.bindValue(Q_UTF8(":musicId"), music_id);
    IfFailureThrow1(query);
}

void Database::removeMusic(QString const& file_path) {
	QSqlQuery query;

	query.prepare(Q_UTF8("SELECT musicId FROM musics WHERE path = (:path)"));
	query.bindValue(Q_UTF8(":path"), file_path);
	
	query.exec();

	while (query.next()) {		
		auto music_id = query.value(Q_UTF8("musicId")).toInt();
		removePlaylistMusic(1, QVector<int32_t>{ music_id });
		removeAlbumMusicId(music_id);
		removeAlbumArtistId(music_id);
		removeMusic(music_id);
		return;
	}
}

void Database::removeAlbumArtist(int32_t album_id) {
    QSqlQuery query;
    query.prepare(Q_UTF8("DELETE FROM albumArtist WHERE albumId=:albumId"));
    query.bindValue(Q_UTF8(":albumId"), album_id);
    IfFailureThrow1(query);
}

void Database::removeAllArtist() {
	QSqlQuery query;
	query.prepare(Q_UTF8("DELETE FROM artists"));
    IfFailureThrow1(query);
}

void Database::removeArtistId(int32_t artist_id) {
    QSqlQuery query;
    query.prepare(Q_UTF8("DELETE FROM artists WHERE artistId=:artistId"));
    query.bindValue(Q_UTF8(":artistId"), artist_id);
    IfFailureThrow1(query);
}

void Database::removeAlbum(int32_t album_id) {
    forEachAlbumMusic(album_id, [this](auto const entity) {
        removePlaylistMusic(1, QVector<int32_t>{ entity.music_id });
		removePlaylistMusic(2, QVector<int32_t>{ entity.music_id });
        removeAlbumMusicId(entity.music_id);
        //removeAlbumArtistId(entity.artist_id);
        removeMusic(entity.music_id);
    });

    removeAlbumArtist(album_id);

    QSqlQuery query;
    query.prepare(Q_UTF8("DELETE FROM albums WHERE albumId=:albumId"));
    query.bindValue(Q_UTF8(":albumId"), album_id);
    IfFailureThrow1(query);
}

int32_t Database::addTable(const QString& name, int32_t table_index, int32_t playlist_id) {
	QSqlTableModel model(nullptr, db_);

	model.setEditStrategy(QSqlTableModel::OnManualSubmit);
	model.setTable(Q_UTF8("tables"));
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
	model.setTable(Q_UTF8("playlist"));
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

	query.prepare(Q_UTF8("UPDATE tables SET name = :name WHERE (tableId = :tableId)"));

	query.bindValue(Q_UTF8(":tableId"), table_id);
	query.bindValue(Q_UTF8(":name"), name);
    IfFailureThrow1(query);
}

void Database::setAlbumCover(int32_t album_id, const QString& album, const QString& cover_id) {
	QSqlQuery query;

	query.prepare(Q_UTF8("UPDATE albums SET coverId = :coverId WHERE (albumId = :albumId) OR (album = :album)"));

	query.bindValue(Q_UTF8(":albumId"), album_id);
	query.bindValue(Q_UTF8(":album"), album);
	query.bindValue(Q_UTF8(":coverId"), cover_id);

    IfFailureThrow1(query);
}

std::optional<ArtistStats> Database::getArtistStats(int32_t artist_id) const {
	QSqlQuery query;

	query.prepare(Q_UTF8(R"(
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

	query.bindValue(Q_UTF8(":artistId"), artist_id);

	IfFailureThrow1(query);

	while (query.next()) {
		ArtistStats stats;
		stats.albums = query.value(Q_UTF8("albums")).toInt();
		stats.tracks = query.value(Q_UTF8("tracks")).toInt();
		stats.durations = query.value(Q_UTF8("durations")).toDouble();
		return stats;
	}

	return std::nullopt;
}

std::optional<AlbumStats> Database::getAlbumStats(int32_t album_id) const {
	QSqlQuery query;

	query.prepare(Q_UTF8(R"(
        SELECT
            SUM(musics.duration) AS durations,
            (SELECT COUNT( * ) AS tracks FROM albumMusic WHERE albumMusic.albumId = :albumId) AS tracks
        FROM
	        albumMusic
	    JOIN albums ON albums.albumId = albumMusic.albumId 
        JOIN musics ON musics.musicId = albumMusic.musicId 
        WHERE
	        albums.albumId = :albumId;)"));

	query.bindValue(Q_UTF8(":albumId"), album_id);

    IfFailureThrow1(query);

	while (query.next()) {
		AlbumStats stats;
		stats.tracks = query.value(Q_UTF8("tracks")).toInt();
		stats.durations = query.value(Q_UTF8("durations")).toDouble();
		return stats;
	}

	return std::nullopt;
}

bool Database::isPlaylistExist(int32_t playlist_id) const {
	QSqlQuery query;

	query.prepare(Q_UTF8("SELECT playlistId FROM playlist WHERE playlistId = (:playlistId)"));
	query.bindValue(Q_UTF8(":playlistId"), playlist_id);

    IfFailureThrow1(query);
	return query.next();
}

int32_t Database::findTablePlaylistId(int32_t table_id) const {
	QSqlQuery query;

	query.prepare(Q_UTF8("SELECT playlistId FROM tablePlaylist WHERE tableId = (:tableId)"));
	query.bindValue(Q_UTF8(":tableId"), table_id);

    IfFailureThrow1(query);
	while (query.next()) {
		return query.value(Q_UTF8("playlistId")).toInt();
	}
	return kInvalidId;
}

void Database::addTablePlaylist(int32_t tableId, int32_t playlist_id) {
	QSqlQuery query;

	query.prepare(Q_UTF8(R"(
                         INSERT INTO tablePlaylist (playlistId, tableId) VALUES (:playlistId, :tableId)
                         )"));

	query.bindValue(Q_UTF8(":playlistId"), playlist_id);
	query.bindValue(Q_UTF8(":tableId"), tableId);

    IfFailureThrow1(query);
}

QString Database::getArtistCoverId(int32_t artist_id) const {
	QSqlQuery query;

	query.prepare(Q_UTF8("SELECT coverId FROM artists WHERE artistId = (:artistId)"));
	query.bindValue(Q_UTF8(":artistId"), artist_id);

    IfFailureThrow1(query);

	const auto index = query.record().indexOf(Q_UTF8("coverId"));
	if (query.next()) {
		return query.value(index).toString();
	}
	return QString();
}

QString Database::getAlbumCoverId(int32_t album_id) const {
	QSqlQuery query;

	query.prepare(Q_UTF8("SELECT coverId FROM albums WHERE albumId = (:albumId)"));
	query.bindValue(Q_UTF8(":albumId"), album_id);

    IfFailureThrow1(query);

	const auto index = query.record().indexOf(Q_UTF8("coverId"));
	if (query.next()) {
		return query.value(index).toString();
	}
	return QString();
}

int32_t Database::addOrUpdateMusic(const Metadata& metadata, int32_t playlist_id) {
	QSqlQuery query;

	query.prepare(
	Q_UTF8(
	R"(
    INSERT OR REPLACE INTO musics
    (musicId, title, track, path, fileExt, fileName, duration, durationStr, parentPath, bitrate, samplerate, offset, dateTime)
    VALUES ((SELECT musicId FROM musics WHERE path = :path and offset = :offset), :title, :track, :path, :fileExt, :fileName, :duration, :durationStr, :parentPath, :bitrate, :samplerate, :offset, :dateTime)
    )")
	);

	query.bindValue(Q_UTF8(":title"), QString::fromStdWString(metadata.title));
	query.bindValue(Q_UTF8(":track"), metadata.track);
	query.bindValue(Q_UTF8(":path"), QString::fromStdWString(metadata.file_path));
	query.bindValue(Q_UTF8(":fileExt"), QString::fromStdWString(metadata.file_ext));
	query.bindValue(Q_UTF8(":fileName"), QString::fromStdWString(metadata.file_name));
	query.bindValue(Q_UTF8(":parentPath"), QString::fromStdWString(metadata.parent_path));
	query.bindValue(Q_UTF8(":duration"), metadata.duration);
	query.bindValue(Q_UTF8(":durationStr"), Time::msToString(metadata.duration));
	query.bindValue(Q_UTF8(":bitrate"), metadata.bitrate);
	query.bindValue(Q_UTF8(":samplerate"), metadata.samplerate);
	query.bindValue(Q_UTF8(":offset"), metadata.offset);
    if (metadata.timestamp == 0) {
        query.bindValue(Q_UTF8(":dateTime"), QDateTime::currentSecsSinceEpoch());
	} else {
		query.bindValue(Q_UTF8(":dateTime"), metadata.timestamp);
	}

	db_.transaction();

	if (!query.exec()) {
		qDebug() << query.lastError().text();
		return kInvalidId;
	}

	auto music_id = query.lastInsertId().toInt();

	if (playlist_id != -1) {
		addMusicToPlaylist(music_id, playlist_id);
	}

	db_.commit();
	return music_id;
}

void Database::updateMusicFingerprint(int32_t music_id, const QString& fingerprint) {
	QSqlQuery query;

	query.prepare(Q_UTF8("UPDATE musics SET fingerprint = :fingerprint WHERE (musicId = :musicId)"));

	query.bindValue(Q_UTF8(":musicId"), music_id);
	query.bindValue(Q_UTF8(":fingerprint"), fingerprint);

	query.exec();
}

void Database::updateMusicFilePath(int32_t music_id, const QString& file_path) {
	QSqlQuery query;

	query.prepare(Q_UTF8("UPDATE musics SET path = :path WHERE (musicId = :musicId)"));

	query.bindValue(Q_UTF8(":musicId"), music_id);
	query.bindValue(Q_UTF8(":path"), file_path);

	query.exec();
}

void Database::updateMusicRating(int32_t music_id, int32_t rating) {
	QSqlQuery query;

	query.prepare(Q_UTF8("UPDATE musics SET rating = :rating WHERE (musicId = :musicId)"));

	query.bindValue(Q_UTF8(":musicId"), music_id);
	query.bindValue(Q_UTF8(":rating"), rating);

    IfFailureThrow1(query);
}

void Database::updateLUFS(int32_t music_id, double lufs, double true_peak) {
    QSqlQuery query;

    query.prepare(Q_UTF8("UPDATE musics SET lufs = :lufs, true_peak = :true_peak WHERE (musicId = :musicId)"));

    query.bindValue(Q_UTF8(":musicId"), music_id);
    query.bindValue(Q_UTF8(":lufs"), lufs);
	query.bindValue(Q_UTF8(":true_peak"), true_peak);

    IfFailureThrow1(query);
}

void Database::addMusicToPlaylist(int32_t music_id, int32_t playlist_id) const {
	QSqlQuery query;

	query.prepare(Q_UTF8(R"(
                         INSERT INTO playlistMusics (playlistMusicsId, playlistId, musicId) VALUES (NULL, :playlistId, :musicId)
                         )"));

	query.bindValue(Q_UTF8(":playlistId"), playlist_id);
	query.bindValue(Q_UTF8(":musicId"), music_id);
    IfFailureThrow1(query);
}

void Database::updateDiscogsArtistId(int32_t artist_id, const QString& discogs_artist_id) {
	QSqlQuery query;

	query.prepare(Q_UTF8("UPDATE artists SET discogsArtistId = :discogsArtistId WHERE (artistId = :artistId)"));

	query.bindValue(Q_UTF8(":artistId"), artist_id);
	query.bindValue(Q_UTF8(":discogsArtistId"), discogs_artist_id);

    IfFailureThrow1(query);
}

void Database::updateArtistMbid(int32_t artist_id, const QString& mbid) {
	QSqlQuery query;

	query.prepare(Q_UTF8("UPDATE artists SET mbid = :mbid WHERE (artistId = :artistId)"));

	query.bindValue(Q_UTF8(":artistId"), artist_id);
	query.bindValue(Q_UTF8(":mbid"), mbid);

    IfFailureThrow1(query);
}

void Database::updateArtistCoverId(int32_t artist_id, const QString& coverId) {
	QSqlQuery query;

	query.prepare(Q_UTF8("UPDATE artists SET coverId = :coverId WHERE (artistId = :artistId)"));

	query.bindValue(Q_UTF8(":artistId"), artist_id);
	query.bindValue(Q_UTF8(":coverId"), coverId);

    IfFailureThrow1(query);
}

int32_t Database::addOrUpdateArtist(const QString& artist) {
	QSqlQuery query;

	query.prepare(
		Q_UTF8(
			R"(
                    INSERT OR REPLACE INTO artists (artistId, artist, firstChar)
                    VALUES ((SELECT artistId FROM artists WHERE artist = :artist), :artist, :firstChar)
                    )"));

	auto firstChar = artist.left(1);
	query.bindValue(Q_UTF8(":artist"), artist);
	query.bindValue(Q_UTF8(":firstChar"), firstChar.toUpper());

    IfFailureThrow1(query);

	const auto artist_id = query.lastInsertId().toInt();
	return artist_id;
}

int32_t Database::addOrUpdateAlbum(const QString& album, int32_t artist_id) {
	QSqlQuery query;

	query.prepare(
		Q_UTF8(R"(
                       INSERT OR REPLACE INTO albums (albumId, album, artistId, firstChar)
                       VALUES ((SELECT albumId FROM albums WHERE album = :album), :album, :artistId, :firstChar)
                       )"));

	auto firstChar = album.left(1);

	query.bindValue(Q_UTF8(":album"), album);
	query.bindValue(Q_UTF8(":artistId"), artist_id);
	query.bindValue(Q_UTF8(":firstChar"), firstChar.toUpper());

    IfFailureThrow1(query);

	const auto album_id = query.lastInsertId().toInt();
	return album_id;
}

void Database::addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) {
	QSqlQuery query;

	query.prepare(
		Q_UTF8(R"(
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

	query.bindValue(Q_UTF8(":album"), album_id);
	query.bindValue(Q_UTF8(":artist"), artist_id);
	query.bindValue(Q_UTF8(":musicId"), music_id);

	db_.transaction();

    IfFailureThrow1(query);

	if (!query.next()) {
		addAlbumMusic(album_id, artist_id, music_id);
		db_.commit();
	}
}

void Database::addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const {
	QSqlQuery query;

	query.prepare(Q_UTF8(R"(
                         INSERT OR REPLACE INTO albumArtist (albumArtistId, albumId, artistId)
                         VALUES ((SELECT albumArtistId from albumArtist where albumId = :albumId AND artistId = :artistId), :albumId, :artistId)
                         )"));

	query.bindValue(Q_UTF8(":albumId"), album_id);
	query.bindValue(Q_UTF8(":artistId"), artist_id);

    IfFailureThrow1(query);
}

void Database::addAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const {
	QSqlQuery query;

	query.prepare(Q_UTF8(R"(
                         INSERT OR REPLACE INTO albumMusic (albumMusicId, albumId, artistId, musicId)
                         VALUES ((SELECT albumMusicId from albumMusic where albumId = :albumId AND artistId = :artistId AND musicId = :musicId), :albumId, :artistId, :musicId)
                         )"));

	query.bindValue(Q_UTF8(":albumId"), album_id);
	query.bindValue(Q_UTF8(":artistId"), artist_id);
	query.bindValue(Q_UTF8(":musicId"), music_id);

    IfFailureThrow1(query);

	addOrUpdateAlbumArtist(album_id, artist_id);
}

void Database::removePlaylistAllMusic(int32_t playlist_id) {
	QSqlQuery query;
	query.prepare(Q_UTF8("DELETE FROM playlistMusics WHERE playlistId=:playlistId"));
	query.bindValue(Q_UTF8(":playlistId"), playlist_id);
	IfFailureThrow1(query);
}

void Database::removePlaylistMusic(int32_t playlist_id, const QVector<int32_t>& select_music_ids) {
	QSqlQuery query;

	QString str = Q_UTF8("DELETE FROM playlistMusics WHERE playlistId=:playlistId AND musicId in (%0)");

	QStringList list;
	for (auto id : select_music_ids) {
		list << QString::number(id);
	}

	auto q = str.arg(list.join(Q_UTF8(",")));
	query.prepare(q);

	query.bindValue(Q_UTF8(":playlistId"), playlist_id);
    IfFailureThrow1(query);
}
