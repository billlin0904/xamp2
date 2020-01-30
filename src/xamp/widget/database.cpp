#include <QSqlTableModel>
#include <QSqlRecord>
#include <QDebug>

#include <base/memory.h>

#include "time_utilts.h"
#include "database.h"

#define ThrowlfFailue(query) \
    do {\
    if (!query.exec()) {\
    throw SqlException(query.lastError());\
    }\
    } while (false);

#define IfFailureThrow(query, sql) \
    do {\
    if (!query.exec(sql)) {\
    throw SqlException(query.lastError());\
    }\
    } while (false);

SqlException::SqlException(QSqlError error)
    : xamp::base::Exception(xamp::base::Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, error.text().toStdString()) {
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
                       playlistId integer,
                       musicId integer,
                       FOREIGN KEY(playlistId) REFERENCES playlist(playlistId),
                       FOREIGN KEY(musicId) REFERENCES musics(musicId)
                       )
                       )"));

    QSqlQuery query(db_);
    for (const auto& sql : create_table_sql) {
        IfFailureThrow(query, sql)
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
    (void)db_.exec(Q_UTF8("PRAGMA locking_mode = EXCLUSIVE"));

    createTableIfNotExist();
}

int32_t Database::addTable(const QString& name, int32_t table_index, int32_t playlist_id) {
    QSqlTableModel model(nullptr, db_);

    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setTable(Q_UTF8("tables"));
    model.select();

    if (!model.insertRows(0, 1)) {
        return INVALID_DATABASE_ID;
    }

    model.setData(model.index(0, 0), QVariant());
    model.setData(model.index(0, 1), table_index);
    model.setData(model.index(0, 2), playlist_id);
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
    model.setTable(Q_UTF8("playlist"));
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

void Database::setTableName(int32_t table_id, const QString &name) {
    QSqlQuery query;

    query.prepare(Q_UTF8("UPDATE tables SET name = :name WHERE (tableId = :tableId)"));

    query.bindValue(Q_UTF8(":tableId"), table_id);
    query.bindValue(Q_UTF8(":name"), name);
    ThrowlfFailue(query)
}

void Database::setAlbumCover(int32_t album_id, const QString& album, const QString& cover_id) {
    QSqlQuery query;

    query.prepare(Q_UTF8("UPDATE albums SET coverId = :coverId WHERE (albumId = :albumId) OR (album = :album)"));

    query.bindValue(Q_UTF8(":albumId"), album_id);
    query.bindValue(Q_UTF8(":album"), album);
    query.bindValue(Q_UTF8(":coverId"), cover_id);

    ThrowlfFailue(query)
}

bool Database::isPlaylistExist(int32_t playlist_id) const {
    QSqlQuery query;

    query.prepare(Q_UTF8("SELECT playlistId FROM playlist WHERE playlistId = (:playlistId)"));
    query.bindValue(Q_UTF8(":playlistId"), playlist_id);

    ThrowlfFailue(query)
    return query.next();
}

int32_t Database::findTablePlaylistId(int32_t table_id) const {
    QSqlQuery query;

    query.prepare(Q_UTF8("SELECT playlistId FROM tablePlaylist WHERE tableId = (:tableId)"));
    query.bindValue(Q_UTF8(":tableId"), table_id);

    ThrowlfFailue(query)
    while (query.next()) {
        return query.value(Q_UTF8("playlistId")).toInt();
    }
    return INVALID_DATABASE_ID;
}

void Database::addTablePlaylist(int32_t tableId, int32_t playlist_id) {
    QSqlQuery query;

    query.prepare(Q_UTF8(R"(
                         INSERT INTO tablePlaylist (playlistId, tableId) VALUES (:playlistId, :tableId)
                         )"));

    query.bindValue(Q_UTF8(":playlistId"), playlist_id);
    query.bindValue(Q_UTF8(":tableId"), tableId);

    ThrowlfFailue(query)
}

QString Database::getAlbumCoverId(int32_t album_id) const {
    QSqlQuery query;

    query.prepare(Q_UTF8("SELECT coverId FROM albums WHERE albumId = (:albumId)"));
    query.bindValue(Q_UTF8(":albumId"), album_id);

    ThrowlfFailue(query)

    const auto album_cover_tag_id_index = query.record().indexOf(Q_UTF8("coverId"));
    if (query.next()) {
        return query.value(album_cover_tag_id_index).toString();
    }
    return QString();
}

int32_t Database::addOrUpdateMusic(const xamp::base::Metadata& metadata, int32_t playlist_id) {
    QSqlQuery query;

    query.prepare(
                Q_UTF8(
                    R"(
                    INSERT OR REPLACE INTO musics
                    (musicId, title, track, path, fileExt, fileName, duration, durationStr, parentPath, bitrate, samplerate, offset)
                    VALUES ((SELECT musicId FROM musics WHERE path = :path and offset = :offset), :title, :track, :path, :fileExt, :fileName, :duration, :durationStr, :parentPath, :bitrate, :samplerate, :offset)
                    )")
                );

    auto album = QString::fromStdWString(metadata.album);
    auto artist = QString::fromStdWString(metadata.artist);

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

    db_.transaction();

    if (!query.exec()) {
        qDebug() << query.lastError().text();
        return INVALID_DATABASE_ID;
    }

    auto music_id = query.lastInsertId().toInt();

    if (playlist_id != -1) {
        addMusicToPlaylist(music_id, playlist_id);
    }    

    db_.commit();
    return music_id;
}

void Database::addMusicToPlaylist(int32_t music_id, int32_t playlist_id) const {
    QSqlQuery query;

    query.prepare(Q_UTF8(R"(
                         INSERT INTO playlistMusics (playlistId, musicId) VALUES (:playlistId, :musicId)
                         )"));

    query.bindValue(Q_UTF8(":playlistId"), playlist_id);
    query.bindValue(Q_UTF8(":musicId"), music_id);
    ThrowlfFailue(query)
}

void Database::updateDiscogsArtistId(int32_t artist_id, const QString& discogs_artist_id) {
    QSqlQuery query;

    query.prepare(Q_UTF8("UPDATE artists SET discogsArtistId = :discogsArtistId WHERE (artistId = :artistId)"));

    query.bindValue(Q_UTF8(":artistId"), artist_id);
    query.bindValue(Q_UTF8(":discogsArtistId"), discogs_artist_id);

    ThrowlfFailue(query)
}

void Database::updateArtistCoverId(int32_t artist_id, const QString &coverId) {
    QSqlQuery query;

    query.prepare(Q_UTF8("UPDATE artists SET coverId = :coverId WHERE (artistId = :artistId)"));

    query.bindValue(Q_UTF8(":artistId"), artist_id);
    query.bindValue(Q_UTF8(":coverId"), coverId);

    ThrowlfFailue(query)
}

int32_t Database::addOrUpdateArtist(const QString& artist) {
    QSqlQuery query;

    query.prepare(
                Q_UTF8(
                    R"(
                    INSERT OR REPLACE INTO artists (artistId, artist)
                    VALUES ((SELECT artistId FROM artists WHERE artist = :artist), :artist)
                    )"));

    query.bindValue(Q_UTF8(":artist"), artist);

    ThrowlfFailue(query)

    const auto artist_id = query.lastInsertId().toInt();
    return artist_id;
}

int32_t Database::addOrUpdateAlbum(const QString& album, int32_t artist_id) {
    QSqlQuery query;

    query.prepare(
                Q_UTF8(R"(
                       INSERT OR REPLACE INTO albums (albumId, album, artistId)
                       VALUES ((SELECT albumId FROM albums WHERE album = :album), :album, :artistId)
                       )"));

    query.bindValue(Q_UTF8(":album"), album);
    query.bindValue(Q_UTF8(":artistId"), artist_id);

    ThrowlfFailue(query)

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

    ThrowlfFailue(query)

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

    ThrowlfFailue(query)
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

    ThrowlfFailue(query)

    addOrUpdateAlbumArtist(album_id, artist_id);
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
    ThrowlfFailue(query)
}
