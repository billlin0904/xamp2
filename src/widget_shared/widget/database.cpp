#include <widget/database.h>

#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDateTime>
#include <QDir>
#include <QThread>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/assert.h>
#include <widget/util/str_utilts.h>

#define DbIfFailedThrow(query, sql) \
    do {\
    if (!query.exec(sql)) {\
    throw SqlException(query.lastError());\
    }\
    } while (false)

#define DbIfFailedThrow1(query) \
    do {\
    if (!query.exec()) {\
    throw SqlException(query.lastError());\
    }\
    } while (false)

namespace {
#define SCHAMA_MIGRATIONS_TABLE "__qt_schema_migrations"

    void printSqlError(const QSqlQuery& query) {
        XAMP_LOG_DEBUG("{}", query.lastError().text().toStdString());
    }

    void createInternalTable(QSqlDatabase& database) {
        QSqlQuery query(qTEXT("create table if not exists " SCHAMA_MIGRATIONS_TABLE " ("
            "version Text primary key not null, "
            "run_on timestamp not null default current_timestamp)"), database);
        if (!query.exec()) {
            printSqlError(query);
        }
    }

    void markMigrationRun(QSqlDatabase& database, const QString& name) {
        XAMP_LOG_DEBUG("Marking migration {} as done.", name.toStdString());

        QSqlQuery query(database);
        if (!query.prepare(qTEXT("insert into " SCHAMA_MIGRATIONS_TABLE " (version) values (:name)"))) {
            printSqlError(query);
        }
        query.bindValue(qTEXT(":name"), name);
        if (!query.exec()) {
            printSqlError(query);
        }
    }

    QString currentDatabaseVersion(QSqlDatabase& database) {
        QSqlQuery query(database);
        query.prepare(qTEXT("select version from " SCHAMA_MIGRATIONS_TABLE " order by version desc limit 1"));
        query.exec();

        if (query.next()) {
            return query.value(0).toString();
        }
        else {
            return {};
        }
    }

    void runDatabaseMigrations(QSqlDatabase& database, const QString& migration_directory) {
        createInternalTable(database);

        QDir dir(migration_directory);
        const auto entries = dir.entryList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot, QDir::SortFlag::Name);

        const QString currentVersion = currentDatabaseVersion(database);
        for (const auto& entry : entries) {
            QDir subdir(entry);
            if (subdir.dirName() > currentVersion) {
                QFile file(migration_directory + QDir::separator() + entry + QDir::separator() + qTEXT("up.sql"));
                if (!file.open(QFile::ReadOnly)) {
                    XAMP_LOG_DEBUG("Failed to open migration file {}", file.fileName().toStdString());
                }
                XAMP_LOG_DEBUG("Running migration {}", subdir.dirName().toStdString());

                database.transaction();

                // Hackish
                const auto statements = file.readAll().split(';');

                bool migration_successful = true;
                for (const QByteArray& statement : statements) {
                    const auto trimmed_statement = QString::fromUtf8(statement.trimmed());
                    QSqlQuery query(database);

                    if (!trimmed_statement.isEmpty()) {
                        XAMP_LOG_DEBUG("Running {}", trimmed_statement.toStdString());
                        if (!query.prepare(trimmed_statement)) {
                            printSqlError(query);
                            migration_successful = false;
                        }
                        else {
                            const bool success = query.exec();
                            migration_successful &= success;
                            if (!success) {
                                printSqlError(query);
                            }
                        }
                    }
                }

                if (migration_successful) {
                    database.commit();
                    markMigrationRun(database, subdir.dirName());
                }
                else {
                    XAMP_LOG_DEBUG("Migration {} failed, retrying next time.", subdir.dirName().toStdString());
                    XAMP_LOG_DEBUG("Stopping migrations here, as the next migration may depens on this one.");
                    database.rollback();
                    return;
                }
            }
        }
        XAMP_LOG_DEBUG("Migrations finished");
    }
}

QString DatabaseFactory::getDatabaseId() {
    return qTEXT("xamp_db_") + QString::number(reinterpret_cast<quint64>(QThread::currentThread()), 16);
}

PooledDatabasePtr getPooledDatabase(int32_t pool_size) {
    return std::make_shared<ObjectPool<Database, DatabaseFactory>>(pool_size);
}

SqlException::SqlException(QSqlError error)
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR,
        error.text().toStdString()) {
    XAMP_LOG_DEBUG("SqlException: {}{}", error.text().toStdString(), GetStackTrace());
}

const char* SqlException::what() const noexcept {
    return message_.c_str();
}

XAMP_DECLARE_LOG_NAME(Database);

Database::Database(const QString& name) {
    logger_ = XampLoggerFactory.GetLogger(kDatabaseLoggerName);
    if (QSqlDatabase::contains(name)) {
        db_ = QSqlDatabase::database(name);
    }
    else {
        db_ = QSqlDatabase::addDatabase(qTEXT("QSQLITE"), name);
    }
    connection_name_ = name;
}

Database::Database()
	: Database(qTEXT("UI")) {
}

QSqlDatabase& Database::database() {
    return db_;
}

Database::~Database() {
    close();
}

void Database::close() {
    if (db_.isOpen()) {
        XAMP_LOG_I(logger_, "Database {} closed.", connection_name_.toStdString());
        db_.close();
    }
}

void Database::open() {    
    db_.setDatabaseName(qTEXT("xamp.db"));

    if (!db_.open()) {
        throw SqlException(db_.lastError());
    }

    XAMP_LOG_I(logger_, "Database {} opened, SQlite version: {}.",
        connection_name_.toStdString(), getVersion().toStdString());

    if (connection_name_ != qTEXT("UI")) {
        return;
    }

    (void)db_.exec(qTEXT("PRAGMA auto_vacuum = FULL"));
    (void)db_.exec(qTEXT("PRAGMA foreign_keys = ON"));
    (void)db_.exec(qTEXT("PRAGMA journal_mode = DELETE"));

    /*(void)db_.exec(qTEXT("PRAGMA synchronous = OFF"));    
    (void)db_.exec(qTEXT("PRAGMA auto_vacuum = FULL"));
    (void)db_.exec(qTEXT("PRAGMA foreign_keys = ON"));
    (void)db_.exec(qTEXT("PRAGMA journal_mode = DELETE"));
    (void)db_.exec(qTEXT("PRAGMA cache_size = 40960"));
    (void)db_.exec(qTEXT("PRAGMA temp_store = MEMORY"));
    (void)db_.exec(qTEXT("PRAGMA mmap_size = 40960"));
    (void)db_.exec(qTEXT("PRAGMA busy_timeout = 1000"));*/    

    createTableIfNotExist();
}

bool Database::transaction() {
    return db_.transaction();
}

bool Database::commit() {
    return db_.commit();
}

void Database::rollback() {
    db_.rollback();
}

QString Database::getVersion() const {
    SqlQuery query(db_);
    query.exec(qTEXT("SELECT sqlite_version() AS version;"));
    if (query.next()) {
        return query.value(qTEXT("version")).toString();
    }
    throw SqlException(query.lastError());
}

void Database::createTableIfNotExist() {
    runDatabaseMigrations(db_, qTEXT(":/xamp/migrations/"));
}

bool Database::dropAllTable() {
    SqlQuery drop_query(db_);

    const QString drop_query_string = qTEXT("DROP TABLE IF EXISTS %1");
    auto tableNames = db_.tables();

    tableNames.removeAll(qTEXT("sqlite_sequence"));
    for (const auto& table_name : qAsConst(tableNames)) {
        DbIfFailedThrow(drop_query, drop_query_string.arg(table_name));
    }
    return true;
}

void Database::clearNowPlayingSkipMusicId(int32_t playlist_id, int32_t skip_playlist_music_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("UPDATE playlistMusics SET playing = :playing WHERE (playlistMusicsId != :skipPlaylistMusicsId)"));
    query.bindValue(qTEXT(":playing"), PlayingState::PLAY_CLEAR);
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    query.bindValue(qTEXT(":skipPlaylistMusicsId"), skip_playlist_music_id);
    DbIfFailedThrow1(query);
}

void Database::clearNowPlaying(int32_t playlist_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("UPDATE playlistMusics SET playing = :playing"));
    query.bindValue(qTEXT(":playing"), PlayingState::PLAY_CLEAR);
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    DbIfFailedThrow1(query);
}

void Database::setNowPlayingState(int32_t playlist_id, int32_t playlist_music_id, PlayingState playing) {
    SqlQuery query(db_);
    query.prepare(qTEXT("UPDATE playlistMusics SET playing = :playing WHERE (playlistId = :playlistId AND playlistMusicsId = :playlistMusicsId)"));
    query.bindValue(qTEXT(":playing"), playing);
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    query.bindValue(qTEXT(":playlistMusicsId"), playlist_music_id);
    DbIfFailedThrow1(query);
}

void Database::clearNowPlaying(int32_t playlist_id, int32_t playlist_music_id) {
    setNowPlayingState(playlist_id, playlist_music_id, PlayingState::PLAY_CLEAR);
}

void Database::setNowPlaying(int32_t playlist_id, int32_t playlist_music_id) {
    setNowPlayingState(playlist_id, playlist_music_id, PlayingState::PLAY_PLAYING);
}

void Database::removePlaylistMusics(int32_t music_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM playlistMusics WHERE musicId=:musicId"));
    query.bindValue(qTEXT(":musicId"), music_id);
    DbIfFailedThrow1(query);
}

void Database::removeAlbumMusic(int32_t album_id, int32_t music_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM albumMusic WHERE albumId=:albumId AND musicId=:musicId"));
    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":musicId"), music_id);
    DbIfFailedThrow1(query);
}

void Database::removeAlbumMusicAlbum(int32_t album_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM albumMusic WHERE albumId=:albumId"));
    query.bindValue(qTEXT(":albumId"), album_id);
    DbIfFailedThrow1(query);
}

void Database::removeAlbumCategory(int32_t album_id) {
	SqlQuery query(db_);
	query.prepare(qTEXT("DELETE FROM albumCategories WHERE albumId=:albumId"));
	query.bindValue(qTEXT(":albumId"), album_id);
	DbIfFailedThrow1(query);
}

void Database::removeAlbumMusicId(int32_t music_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM albumMusic WHERE musicId=:musicId"));
    query.bindValue(qTEXT(":musicId"), music_id);
    DbIfFailedThrow1(query);
}

void Database::removeTrackLoudnessMusicId(int32_t music_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM musicLoudness WHERE musicId=:musicId"));
    query.bindValue(qTEXT(":musicId"), music_id);
    DbIfFailedThrow1(query);
}

void Database::removeAlbumArtistId(int32_t artist_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM albumArtist WHERE artistId=:artistId"));
    query.bindValue(qTEXT(":artistId"), artist_id);
    DbIfFailedThrow1(query);
}

void Database::removeMusic(int32_t music_id) const {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM musics WHERE musicId=:musicId"));
    query.bindValue(qTEXT(":musicId"), music_id);
    DbIfFailedThrow1(query);
}

std::optional<QString> Database::getAlbumFirstMusicFilePath(int32_t album_id) const {
    SqlQuery query(qTEXT(R"(
SELECT
    musics.path
FROM
    albumMusic
    LEFT JOIN albums ON albums.albumId = albumMusic.albumId
    LEFT JOIN musics ON musics.musicId = albumMusic.musicId
WHERE
    albums.albumId = ?
LIMIT
    1;
)"), db_);
    query.addBindValue(album_id);
    query.exec();
    while (query.next()) {
        return  std::optional<QString> { std::in_place_t{}, query.value(qTEXT("path")).toString() };
    }
    return std::nullopt;
}

void Database::removeMusic(QString const& file_path) {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT musicId FROM musics WHERE path = (:path)"));
    query.bindValue(qTEXT(":path"), file_path);

    query.exec();

    while (query.next()) {
	    const auto music_id = query.value(qTEXT("musicId")).toInt();
        removePlaylistMusic(1, QVector<int32_t>{ music_id });
        removeAlbumMusicId(music_id);
        removeTrackLoudnessMusicId(music_id);
        removeAlbumArtistId(music_id);
        removeMusic(music_id);
        return;
    }
}

void Database::removeAlbumArtist(int32_t album_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM albumArtist WHERE albumId=:albumId"));
    query.bindValue(qTEXT(":albumId"), album_id);
    DbIfFailedThrow1(query);
}

int32_t Database::getAlbumIdFromAlbumMusic(int32_t music_id) {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT albumId FROM albumMusic WHERE musicId = (:musicId)"));
    query.bindValue(qTEXT(":musicId"), music_id);
    query.exec();

    if (query.next()) {
        return query.value(qTEXT("albumId")).toInt();
    }
    return kInvalidDatabaseId;
}

void Database::forEachPlaylist(std::function<void(int32_t, int32_t, StoreType, QString, QString)>&& fun) {
    QSqlTableModel model(nullptr, db_);

    model.setTable(qTEXT("playlist"));
    model.setSort(1, Qt::AscendingOrder);
    model.select();

    for (auto i = 0; i < model.rowCount(); ++i) {
        auto record = model.record(i);
        fun(record.value(qTEXT("playlistId")).toInt(),
            record.value(qTEXT("playlistIndex")).toInt(),
            static_cast<StoreType>(record.value(qTEXT("storeType")).toInt()),
            record.value(qTEXT("cloudPlaylistId")).toString(),
            record.value(qTEXT("name")).toString());
    }
}

void Database::forEachAlbumCover(std::function<void(QString)>&& fun, int limit) const {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
SELECT
    albums.coverId
FROM
    albums
LEFT 
	JOIN artists ON artists.artistId = albums.artistId
WHERE 
	albums.storeType = -1
LIMIT
    :limit
    )"));
    query.bindValue(qTEXT(":limit"), limit);
    DbIfFailedThrow1(query);
    while (query.next()) {
        fun(query.value(qTEXT("coverId")).toString());
    }
}

void Database::forEachAlbumMusic(int32_t album_id, std::function<void(PlayListEntity const&)>&& fun) {
    SqlQuery query(qTEXT(R"(
SELECT
    albumMusic.albumId,
    albumMusic.artistId,
    musicLoudness.trackLoudness,
    albums.album,
    albums.coverId,
    artists.artist,
    musics.*,
    albums.discId
FROM
    albumMusic
    LEFT JOIN albums ON albums.albumId = albumMusic.albumId
    LEFT JOIN artists ON artists.artistId = albumMusic.artistId
    LEFT JOIN musics ON musics.musicId = albumMusic.musicId
    LEFT JOIN musicLoudness ON musicLoudness.musicId = albumMusic.musicId
WHERE
    albums.albumId = ?
)"), db_);
    query.addBindValue(album_id);

    if (!query.exec()) {
        XAMP_LOG_D(logger_, "{}", query.lastError().text().toStdString());
    }

    while (query.next()) {
        fun(fromSqlQuery(query));
    }
}

void Database::removeAllArtist() {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM artists"));
    DbIfFailedThrow1(query);
}

void Database::removeArtistId(int32_t artist_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM artists WHERE artistId=:artistId"));
    query.bindValue(qTEXT(":artistId"), artist_id);
    DbIfFailedThrow1(query);
}

void Database::forEachAlbum(std::function<void(int32_t)>&& fun) {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT albumId FROM albums"));
    DbIfFailedThrow1(query);
    while (query.next()) {
        fun(query.value(qTEXT("albumId")).toInt());
    }
}

void Database::removeAlbum(int32_t album_id) {
    QList<PlayListEntity> entities;
    forEachAlbumMusic(album_id, [this, &entities](auto const& entity) {
        entities.push_back(entity);        
    });

    Q_FOREACH(const auto & entity, entities) {
        QList<int32_t> playlist_ids;
        forEachPlaylist([&playlist_ids, this](auto playlistId, auto, auto, auto, auto) {
            playlist_ids.push_back(playlistId);            
        });

        Q_FOREACH(auto playlistId, playlist_ids) {
            removePlaylistMusic(playlistId, QVector<int32_t>{ entity.music_id });
        }
        removeAlbumCategory(album_id);
        removeAlbumMusicAlbum(album_id);
        removeTrackLoudnessMusicId(entity.music_id);
        removeMusic(entity.music_id);
    }
    
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM albums WHERE albumId=:albumId"));
    query.bindValue(qTEXT(":albumId"), album_id);
    DbIfFailedThrow1(query);
}

QStringList Database::getYears() const {
    QStringList years;

    SqlQuery query(db_);
    query.prepare(qTEXT(R"(
SELECT
    year AS group_year,
    COUNT(*) AS count
FROM albums
WHERE group_year != 0
GROUP BY group_year
ORDER BY group_year DESC;
    )")
    );

    DbIfFailedThrow1(query);

    while (query.next()) {
        auto genre = query.value(qTEXT("group_year")).toString();
        if (genre.isEmpty()) {
            continue;
        }
        years.append(genre);
    }
    return years;
}

QStringList Database::getGenres() const {    
    QStringList genres;

    SqlQuery query(db_);
    query.prepare(qTEXT(R"(
SELECT
    CASE
        WHEN instr(albums.genre, ',') > 0 THEN substr(albums.genre, 1, instr(albums.genre, ',') - 1)
        ELSE albums.genre
    END AS group_name,
    COUNT(*) AS count
FROM albums
GROUP BY group_name
ORDER BY count DESC;
    )")
    );

    DbIfFailedThrow1(query);

    while (query.next()) {
        auto genre = query.value(qTEXT("group_name")).toString();
        if (genre.isEmpty()) {
            continue;
        }
        genres.append(genre);
    }
    return genres;
}

int32_t Database::addPlaylist(const QString& name, int32_t play_index, StoreType store_type, const QString& cloud_playlist_id) {
    QSqlTableModel model(nullptr, db_);

    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setTable(qTEXT("playlist"));
    model.select();

    if (!model.insertRows(0, 1)) {
        return kInvalidDatabaseId;
    }

    model.setData(model.index(0, 0), QVariant());
    model.setData(model.index(0, 1), play_index);
    model.setData(model.index(0, 2), static_cast<int32_t>(store_type));
    model.setData(model.index(0, 3), cloud_playlist_id);
    model.setData(model.index(0, 4), name);

    if (!model.submitAll()) {
        return kInvalidDatabaseId;
    }

    model.database().commit();
    return model.query().lastInsertId().toInt();
}

void Database::setPlaylistIndex(int32_t playlist_id, int32_t play_index, StoreType store_type) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE playlist SET playlistIndex = :playlistIndex WHERE (playlistId = :playlistId)"));

    query.bindValue(qTEXT(":playlistId"), playlist_id);
    query.bindValue(qTEXT(":playlistIndex"), play_index);
    query.bindValue(qTEXT(":storeType"), static_cast<int32_t>(store_type));
    DbIfFailedThrow1(query);
}

std::map<int32_t, int32_t> Database::getPlaylistIndex(StoreType type) {
    std::map<int32_t, int32_t> playlist_index;

    forEachPlaylist([&playlist_index, type](auto id, auto index, auto store_type, auto name, auto) {
        if (type == store_type) {
            playlist_index.insert(std::make_pair(index, id));
        }
        });
    return playlist_index;
}

void Database::setPlaylistName(int32_t playlist_id, const QString& name) {
    SqlQuery query(db_);
    
    query.prepare(qTEXT("UPDATE playlist SET name = :name WHERE (playlistId = :playlistId)"));
    
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    query.bindValue(qTEXT(":name"), name);
    DbIfFailedThrow1(query);
}

void Database::setMusicCover(int32_t music_id, const QString& cover_id) {
    XAMP_ENSURES(!cover_id.isEmpty());

    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET coverId = :coverId WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":coverId"), cover_id);

    DbIfFailedThrow1(query);
    XAMP_LOG_D(logger_, "setMusicCover musicId: {} coverId: {}", music_id, cover_id.toStdString());
}

void Database::setAlbumCover(int32_t album_id, const QString& cover_id) {
    XAMP_ENSURES(!cover_id.isEmpty());

    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE albums SET coverId = :coverId WHERE (albumId = :albumId)"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":coverId"), cover_id);

    DbIfFailedThrow1(query);
    XAMP_LOG_D(logger_, "setAlbumCover albumId: {} coverId: {}", album_id, cover_id.toStdString());
}

std::optional<ArtistStats> Database::getArtistStats(int32_t artist_id) const {    
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
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

    query.bindValue(qTEXT(":artistId"), artist_id);

    DbIfFailedThrow1(query);

    while (query.next()) {
        ArtistStats stats;
        stats.albums = query.value(qTEXT("albums")).toInt();
        stats.tracks = query.value(qTEXT("tracks")).toInt();
        stats.durations = query.value(qTEXT("durations")).toDouble();
        return std::optional<ArtistStats>{ std::in_place_t{},stats };
    }

    return std::nullopt;
}

std::optional<AlbumStats> Database::getAlbumStats(int32_t album_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    SELECT
        SUM(musics.duration) AS durations,
        MAX(year) AS year,
        (SELECT COUNT( * ) AS tracks FROM albumMusic WHERE albumMusic.albumId = :albumId) AS tracks,
        SUM(musics.fileSize) AS fileSize
    FROM
        albumMusic
    JOIN albums ON albums.albumId = albumMusic.albumId
    JOIN musics ON musics.musicId = albumMusic.musicId
    WHERE
        albums.albumId = :albumId;)"));

    query.bindValue(qTEXT(":albumId"), album_id);

    DbIfFailedThrow1(query);

    while (query.next()) {
        AlbumStats stats;
        stats.songs = query.value(qTEXT("tracks")).toInt();
        stats.year = query.value(qTEXT("year")).toInt();
        stats.durations = query.value(qTEXT("durations")).toDouble();
        stats.file_size = query.value(qTEXT("fileSize")).toULongLong();
        return std::optional<AlbumStats>{ std::in_place_t{}, stats };
    }

    return std::nullopt;
}

bool Database::isPlaylistExist(int32_t playlist_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT playlistId FROM playlist WHERE playlistId = (:playlistId)"));
    query.bindValue(qTEXT(":playlistId"), playlist_id);

    DbIfFailedThrow1(query);
    return query.next();
}

QStringList Database::getCategories() const {
    QStringList categories;
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT category FROM albumCategories GROUP BY category"));

    DbIfFailedThrow1(query);

    while (query.next()) {
        categories.append(query.value(qTEXT("category")).toString());
    }

    return categories;
}

QString Database::getArtistCoverId(int32_t artist_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT coverId FROM artists WHERE artistId = (:artistId)"));
    query.bindValue(qTEXT(":artistId"), artist_id);

    DbIfFailedThrow1(query);

    const auto index = query.record().indexOf(qTEXT("coverId"));
    if (query.next()) {
        return query.value(index).toString();
    }
    return kEmptyString;
}

QString Database::getMusicCoverId(int32_t music_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT coverId FROM musics WHERE musicId = (:musicId)"));
    query.bindValue(qTEXT(":musicId"), music_id);

    DbIfFailedThrow1(query);

    const auto index = query.record().indexOf(qTEXT("coverId"));
    if (query.next()) {
        return query.value(index).toString();
    }
    return kEmptyString;
}

QString Database::getMusicFilePath(int32_t music_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT path FROM musics WHERE musicId = (:musicId)"));
    query.bindValue(qTEXT(":musicId"), music_id);

    DbIfFailedThrow1(query);

    const auto index = query.record().indexOf(qTEXT("path"));
    if (query.next()) {
        return query.value(index).toString();
    }
    return kEmptyString;
}

QString Database::getAlbumCoverId(int32_t album_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT coverId FROM albums WHERE albumId = (:albumId)"));
    query.bindValue(qTEXT(":albumId"), album_id);

    DbIfFailedThrow1(query);

    const auto index = query.record().indexOf(qTEXT("coverId"));
    if (query.next()) {
        return query.value(index).toString();
    }
    return kEmptyString;
}

QList<int32_t> Database::getSelectedAlbums() const {
    QList<int32_t> selected_albums;
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT albumId FROM albums WHERE isSelected = 1"));

    DbIfFailedThrow1(query);

    const auto index = query.record().indexOf(qTEXT("albumId"));
    while (query.next()) {
        selected_albums.append(query.value(index).toInt());
    }
    return selected_albums;
}

int32_t Database::getAlbumId(const QString& album) const {
	SqlQuery query(db_);

	query.prepare(qTEXT("SELECT albumId FROM albums WHERE album = (:album)"));
	query.bindValue(qTEXT(":album"), album);

	DbIfFailedThrow1(query);
	const auto index = query.record().indexOf(qTEXT("albumId"));
    if (query.next()) {
		return query.value(index).toInt();
	}
	return kInvalidDatabaseId;
}

QString Database::getAlbumCoverId(const QString& album) const {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT coverId FROM albums WHERE album = (:album)"));
    query.bindValue(qTEXT(":album"), album);

    DbIfFailedThrow1(query);

    const auto index = query.record().indexOf(qTEXT("coverId"));
    if (query.next()) {
        return query.value(index).toString();
    }
    return kEmptyString;
}

PlayListEntity Database::fromSqlQuery(const SqlQuery& query) {
    PlayListEntity entity;
    entity.album_id = query.value(qTEXT("albumId")).toInt();
    entity.artist_id = query.value(qTEXT("artistId")).toInt();
    entity.music_id = query.value(qTEXT("musicId")).toInt();
    entity.file_path = query.value(qTEXT("path")).toString();
    entity.track = query.value(qTEXT("track")).toUInt();
    entity.title = query.value(qTEXT("title")).toString();
    entity.file_name = query.value(qTEXT("fileName")).toString();
    entity.album = query.value(qTEXT("album")).toString();
    entity.artist = query.value(qTEXT("artist")).toString();
    entity.file_extension = query.value(qTEXT("fileExt")).toString();
    entity.parent_path = query.value(qTEXT("parentPath")).toString();
    entity.duration = query.value(qTEXT("duration")).toDouble();
    entity.bit_rate = query.value(qTEXT("bitRate")).toUInt();
    entity.sample_rate = query.value(qTEXT("sampleRate")).toUInt();
    entity.cover_id = query.value(qTEXT("coverId")).toString();
    entity.rating = query.value(qTEXT("rating")).toUInt();
    entity.album_replay_gain = query.value(qTEXT("albumReplayGain")).toDouble();
    entity.album_peak = query.value(qTEXT("albumPeak")).toDouble();
    entity.track_replay_gain = query.value(qTEXT("trackReplayGain")).toDouble();
    entity.track_peak = query.value(qTEXT("trackPeak")).toDouble();
    entity.track_loudness = query.value(qTEXT("trackLoudness")).toDouble();

    entity.genre = query.value(qTEXT("genre")).toString();
    entity.comment = query.value(qTEXT("comment")).toString();
    entity.file_size = query.value(qTEXT("fileSize")).toULongLong();

    entity.lyrc = query.value(qTEXT("lyrc")).toString();
    entity.trlyrc = query.value(qTEXT("trLyrc")).toString();

    QFileInfo file_info(entity.file_path);
    entity.file_extension = file_info.suffix();
    entity.file_name = file_info.completeBaseName();
    entity.parent_path = toNativeSeparators(file_info.dir().path());

    return entity;
}

std::optional<std::tuple<QString, QString>> Database::getLyrics(int32_t music_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT(R"(
    SELECT
        lyrc, trLyrc
    FROM
        musics
    WHERE
        musicId = :musicId
    )")
    );

    query.bindValue(qTEXT(":musicId"), music_id);

    DbIfFailedThrow1(query);

    while (query.next()) {
        auto lyrc = query.value(qTEXT("lyrc")).toString();
        auto tr_lyrc = query.value(qTEXT("trLyrc")).toString();
        if (!lyrc.isEmpty()) {
            return std::optional<std::tuple<QString, QString>> { std::in_place_t{},
                std::tuple<QString, QString> { lyrc, tr_lyrc } };
        }
    }
    return std::nullopt;
}

void Database::updateMusic(int32_t music_id, const TrackInfo& track_info) {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    UPDATE musics SET
    title = :title,
	track = :track,
	path = :path,
	fileExt = :fileExt,
	fileName= :fileName,
	duration= :duration,
	durationStr= :durationStr,
	parentPath= :parentPath,
	bitRate= :bitRate,
	sampleRate= :sampleRate,
	offset= :offset,
	dateTime= :dateTime,
	albumReplayGain= :albumReplayGain,
	trackReplayGain= :trackReplayGain, 
	albumPeak= :albumPeak,
	trackPeak= :trackPeak,
	genre= :genre, 
	comment= :comment,
	fileSize= :fileSize,
	heart= :heart    
	WHERE musicId = :musicId
    )")
    );

    query.bindValue(qTEXT(":title"),       toQString(track_info.title));
    query.bindValue(qTEXT(":track"),       track_info.track);
    query.bindValue(qTEXT(":path"),        toQString(track_info.file_path));
    query.bindValue(qTEXT(":fileExt"),     toQString(track_info.file_ext()));
    query.bindValue(qTEXT(":fileName"),    toQString(track_info.file_name()));
    query.bindValue(qTEXT(":parentPath"),  toQString(track_info.parent_path()));
    query.bindValue(qTEXT(":duration"),    track_info.duration);
    query.bindValue(qTEXT(":durationStr"), formatDuration(track_info.duration));
    query.bindValue(qTEXT(":bitRate"),     track_info.bit_rate);
    query.bindValue(qTEXT(":sampleRate"),  track_info.sample_rate);
    query.bindValue(qTEXT(":offset"),      track_info.offset ? track_info.offset.value() : 0);
    query.bindValue(qTEXT(":fileSize"),    track_info.file_size);
    query.bindValue(qTEXT(":heart"),       track_info.rating ? 1 : 0);

    if (track_info.replay_gain) {
        query.bindValue(qTEXT(":albumReplayGain"), track_info.replay_gain.value().album_gain);
        query.bindValue(qTEXT(":trackReplayGain"), track_info.replay_gain.value().track_gain);
        query.bindValue(qTEXT(":albumPeak"),       track_info.replay_gain.value().album_peak);
        query.bindValue(qTEXT(":trackPeak"),       track_info.replay_gain.value().track_peak);
    }
    else {
        query.bindValue(qTEXT(":albumReplayGain"), QVariant());
        query.bindValue(qTEXT(":trackReplayGain"), QVariant());
        query.bindValue(qTEXT(":albumPeak"), QVariant());
        query.bindValue(qTEXT(":trackPeak"), QVariant());
    }

    query.bindValue(qTEXT(":dateTime"), track_info.last_write_time);
    query.bindValue(qTEXT(":genre"), toQString(track_info.genre));
    query.bindValue(qTEXT(":comment"), toQString(track_info.comment));

    query.bindValue(qTEXT(":musicId"), music_id);

    DbIfFailedThrow1(query);
}

int32_t Database::addOrUpdateMusic(const TrackInfo& track_info) {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO musics
    (musicId, title, track, path, fileExt, fileName, duration, durationStr, parentPath, bitRate, sampleRate, offset, dateTime, albumReplayGain, trackReplayGain, albumPeak, trackPeak, genre, comment, fileSize, heart)
    VALUES ((SELECT musicId FROM musics WHERE path = :path AND offset = :offset), :title, :track, :path, :fileExt, :fileName, :duration, :durationStr, :parentPath, :bitRate, :sampleRate, :offset, :dateTime, :albumReplayGain, :trackReplayGain, :albumPeak, :trackPeak, :genre, :comment, :fileSize, :heart)
    )")
    );

    query.bindValue(qTEXT(":title"), toQString(track_info.title));
    query.bindValue(qTEXT(":track"), track_info.track);
    query.bindValue(qTEXT(":path"), toQString(track_info.file_path));
    query.bindValue(qTEXT(":fileExt"), toQString(track_info.file_ext()));
    query.bindValue(qTEXT(":fileName"), toQString(track_info.file_name()));
    query.bindValue(qTEXT(":parentPath"), toQString(track_info.parent_path()));
    query.bindValue(qTEXT(":duration"), track_info.duration);
    query.bindValue(qTEXT(":durationStr"), formatDuration(track_info.duration));
    query.bindValue(qTEXT(":bitRate"), track_info.bit_rate);
    query.bindValue(qTEXT(":sampleRate"), track_info.sample_rate);
    query.bindValue(qTEXT(":offset"), track_info.offset ? track_info.offset.value() : 0);
    query.bindValue(qTEXT(":fileSize"), track_info.file_size);
    query.bindValue(qTEXT(":heart"), track_info.rating ? 1 : 0);

    if (track_info.replay_gain) {
        query.bindValue(qTEXT(":albumReplayGain"), track_info.replay_gain.value().album_gain);
        query.bindValue(qTEXT(":trackReplayGain"), track_info.replay_gain.value().track_gain);
        query.bindValue(qTEXT(":albumPeak"), track_info.replay_gain.value().album_peak);
        query.bindValue(qTEXT(":trackPeak"), track_info.replay_gain.value().track_peak);
    }
    else {
        query.bindValue(qTEXT(":albumReplayGain"), QVariant());
        query.bindValue(qTEXT(":trackReplayGain"), QVariant());
        query.bindValue(qTEXT(":albumPeak"),       QVariant());
        query.bindValue(qTEXT(":trackPeak"),       QVariant());
    }

    query.bindValue(qTEXT(":dateTime"), track_info.last_write_time);
    query.bindValue(qTEXT(":genre"), toQString(track_info.genre));
    query.bindValue(qTEXT(":comment"), toQString(track_info.comment));

    DbIfFailedThrow1(query);

    const auto music_id = query.lastInsertId().toInt();

    XAMP_LOG_D(logger_, "addOrUpdateMusic musicId:{}", music_id);

    XAMP_ENSURES(music_id != kInvalidDatabaseId);
    return music_id;
}

void Database::addOrUpdateLyrics(int32_t music_id, const QString& lyrc, const QString& trlyrc) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET lyrc = :lyrc, trlyrc = :trlyrc WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":lyrc"), lyrc);
    query.bindValue(qTEXT(":trlyrc"), trlyrc);

    query.exec();
}

void Database::updateMusicFilePath(int32_t music_id, const QString& file_path) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET path = :path WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":path"), file_path);

    query.exec();
}

void Database::updateMusicRating(int32_t music_id, int32_t rating) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET rating = :rating WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":rating"), rating);

    DbIfFailedThrow1(query);
}

void Database::updateAlbumSelectState(int32_t album_id, bool state) {
    SqlQuery query(db_);

    if (album_id != kInvalidDatabaseId) {
        query.prepare(qTEXT("UPDATE albums SET isSelected = :isSelected WHERE (albumId = :albumId) AND storeType == -1"));
        query.bindValue(qTEXT(":albumId"), album_id);
    }
    else {
        query.prepare(qTEXT("UPDATE albums SET isSelected = :isSelected WHERE storeType == -1"));
    }

    query.bindValue(qTEXT(":isSelected"), state ? 1: 0);

    DbIfFailedThrow1(query);
}

void Database::updateAlbum(int32_t album_id, const QString& album) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE albums SET album = :album WHERE (albumId = :albumId)"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":album"), album);

    DbIfFailedThrow1(query);
}

void Database::updateArtist(int32_t artist_id, const QString& artist) {
    
}

void Database::updateAlbumHeart(int32_t album_id, uint32_t heart) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE albums SET heart = :heart WHERE (albumId = :albumId)"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":heart"), heart);

    DbIfFailedThrow1(query);
}

void Database::updateMusicHeart(int32_t music_id, uint32_t heart) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET heart = :heart WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":heart"), heart);

    DbIfFailedThrow1(query);
}

void Database::updateMusicTitle(int32_t music_id, const QString& title) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET title = :title WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":title"), title);

    DbIfFailedThrow1(query);
}

void Database::updateMusicPlays(int32_t music_id) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET plays = plays + 1 WHERE (musicId = :musicId)"));
    query.bindValue(qTEXT(":musicId"), music_id);
    DbIfFailedThrow1(query);
}

void Database::addOrUpdateTrackLoudness(int32_t album_id, int32_t artist_id, int32_t music_id, double track_loudness) {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO musicLoudness (musicLoudnessId, albumId, artistId, musicId, trackLoudness)
    VALUES
    (
    (SELECT musicLoudnessId FROM musicLoudness WHERE albumId = :albumId AND artistId = :artistId AND musicId = :musicId AND trackLoudness = :trackLoudness),
    :albumId,
    :artistId,
    :musicId,
    :trackLoudness
    )
    )"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":artistId"), artist_id);
    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":trackLoudness"), track_loudness);

    DbIfFailedThrow1(query);
}

void Database::updateReplayGain(int32_t music_id,
    double album_rg_gain,
    double album_peak,
    double track_rg_gain,
    double track_peak) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET albumReplayGain = :albumReplayGain, albumPeak = :albumPeak, trackReplayGain = :trackReplayGain, trackPeak = :trackPeak WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":albumReplayGain"), album_rg_gain);
    query.bindValue(qTEXT(":albumPeak"), album_peak);
    query.bindValue(qTEXT(":trackReplayGain"), track_rg_gain);
    query.bindValue(qTEXT(":trackPeak"), track_peak);

    DbIfFailedThrow1(query);
}

void Database::addMusicToPlaylist(int32_t music_id, int32_t playlist_id, int32_t album_id) const {
    SqlQuery query(db_);

    const auto querystr = qSTR("INSERT INTO playlistMusics (playlistMusicsId, playlistId, musicId, albumId) VALUES (NULL, %1, %2, %3)")
        .arg(playlist_id)
        .arg(music_id)
        .arg(album_id);

    query.prepare(querystr);
    DbIfFailedThrow1(query);
}

void Database::addMusicToPlaylist(const QList<int32_t>& music_id, int32_t playlist_id) const {
    if (music_id.isEmpty()) {
        return;
    }
    SqlQuery query(db_);

    QStringList strings;

    for (const auto id : music_id) {
        strings << qTEXT("(") + qTEXT("NULL, ") + QString::number(playlist_id) + qTEXT(", ") + QString::number(id) + qTEXT(")");
    }

    const auto querystr = qTEXT("INSERT INTO playlistMusics (playlistMusicsId, playlistId, musicId) VALUES ")
    + strings.join(qTEXT(","));
    query.prepare(querystr);
    DbIfFailedThrow1(query);
}

void Database::updateArtistEnglishName(const QString& artist, const QString& en_name) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE artists SET artistNameEn = :artistNameEn, firstCharEn = :firstCharEn WHERE (artist = :artist AND firstCharEn IS NULL)"));

    query.bindValue(qTEXT(":artist"), artist);
    query.bindValue(qTEXT(":artistNameEn"), en_name);
    query.bindValue(qTEXT(":firstCharEn"), en_name.left(1));

    DbIfFailedThrow1(query);
}

void Database::updateArtistCoverId(int32_t artist_id, const QString& cover_id) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE artists SET coverId = :coverId WHERE (artistId = :artistId)"));

    query.bindValue(qTEXT(":artistId"), artist_id);
    query.bindValue(qTEXT(":coverId"), cover_id);

    DbIfFailedThrow1(query);
}

int32_t Database::addOrUpdateArtist(const QString& artist) {
    XAMP_EXPECTS(!artist.isEmpty());

    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO artists (artistId, artist, firstChar)
    VALUES ((SELECT artistId FROM artists WHERE artist = :artist), :artist, :firstChar)
    )"));

    const auto first_char = artist.left(1);
    query.bindValue(qTEXT(":artist"), artist);
    query.bindValue(qTEXT(":firstChar"), first_char.toUpper());

    DbIfFailedThrow1(query);

    const auto artist_id = query.lastInsertId().toInt();
    return artist_id;
}

void Database::updateArtistByDiscId(const QString& disc_id, const QString& artist) {
    XAMP_EXPECTS(!artist.isEmpty());

    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    SELECT
        albumMusic.artistId
    FROM
        albumMusic
    JOIN albums ON albums.albumId = albumMusic.albumId
    WHERE
        albums.discId = :discId
    LIMIT 1
    )"));

    query.bindValue(qTEXT(":discId"), disc_id);
    DbIfFailedThrow1(query);

    const auto index = query.record().indexOf(qTEXT("artistId"));
    if (query.next()) {
        auto artist_id = query.value(index).toInt();
        query.prepare(qTEXT("UPDATE artists SET artist = :artist WHERE (artistId = :artistId)"));
        query.bindValue(qTEXT(":artistId"), artist_id);
        query.bindValue(qTEXT(":artist"), artist);
        DbIfFailedThrow1(query);
    }
}

int32_t Database::getAlbumIdByDiscId(const QString& disc_id) const {
    SqlQuery query(db_);
    query.prepare(qTEXT("SELECT albumId FROM albums WHERE discId = (:discId)"));
    query.bindValue(qTEXT(":discId"), disc_id);
    DbIfFailedThrow1(query);

    const auto index = query.record().indexOf(qTEXT("albumId"));
    if (query.next()) {
        return query.value(index).toInt();
    }
    return kInvalidDatabaseId;
}

void Database::updateAlbumByDiscId(const QString& disc_id, const QString& album) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE albums SET album = :album WHERE (discId = :discId)"));

    query.bindValue(qTEXT(":album"), album);
    query.bindValue(qTEXT(":discId"), disc_id);

    DbIfFailedThrow1(query);
}

int32_t Database::addOrUpdateAlbum(const QString& album,
    int32_t artist_id,
    int64_t album_time,
    uint32_t year,
    StoreType store_type, 
    const QString& disc_id, 
    bool is_hires) {
    XAMP_ENSURES(!album.isEmpty());
    XAMP_ENSURES(year != UINT32_MAX);

	SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO albums (
      albumId, 
      album,
      artistId,
      coverId, 
      storeType,
      dateTime,
      discId,
      year,
      isHiRes
    ) 
    VALUES 
      (
        (
          SELECT 
            albumId 
          FROM 
            albums 
          WHERE 
            album = :album
        ), 
        :album, 
        :artistId, 
        :coverId, 
        :storeType, 
        :dateTime, 
        :discId, 
        :year, 
        :isHiRes
      )
    )"));

    query.bindValue(qTEXT(":album"),     album);
    query.bindValue(qTEXT(":artistId"),  artist_id);
    query.bindValue(qTEXT(":coverId"),   getAlbumCoverId(album));
    query.bindValue(qTEXT(":storeType"), static_cast<int32_t>(store_type));
    query.bindValue(qTEXT(":dateTime"),  album_time);
    query.bindValue(qTEXT(":discId"),    disc_id);
    query.bindValue(qTEXT(":year"),      year);
    query.bindValue(qTEXT(":isHiRes"),   is_hires);

    DbIfFailedThrow1(query);

    const auto album_id = query.lastInsertId().toInt();

    XAMP_LOG_D(logger_, "addOrUpdateAlbum albumId:{}", album_id);
    return album_id;
}

void Database::addAlbumCategory(int32_t album_id, const QString& category) const {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT INTO albumCategories (albumCategoryId, albumId, category)
    VALUES (NULL, :albumId, :category)
    )"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":category"), category);

    DbIfFailedThrow1(query);
}

void Database::addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT INTO albumArtist (albumArtistId, albumId, artistId)
    VALUES (NULL, :albumId, :artistId)
    )"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":artistId"), artist_id);

    DbIfFailedThrow1(query);

    XAMP_LOG_D(logger_, "addOrUpdateAlbumArtist albumId:{} artistId:{}", album_id, artist_id);
}

void Database::addOrUpdateMusicLoudness(int32_t album_id, int32_t artist_id, int32_t music_id, double track_loudness) const {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO musicLoudness (albumMusicId, albumId, artistId, musicId, trackLoudness)
    VALUES ((SELECT albumMusicId from musicLoudness where albumId = :albumId AND artistId = :artistId AND musicId = :musicId), :albumId, :artistId, :musicId, :trackLoudness)
    )"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":artistId"), artist_id);
    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":trackLoudness"), track_loudness);

    DbIfFailedThrow1(query);
}

void Database::addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const {
    XAMP_EXPECTS(album_id != kInvalidDatabaseId);
    XAMP_EXPECTS(artist_id != kInvalidDatabaseId);
    XAMP_EXPECTS(music_id != kInvalidDatabaseId);

    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO albumMusic (albumMusicId, albumId, artistId, musicId)
    VALUES ((SELECT albumMusicId from albumMusic where albumId = :albumId AND artistId = :artistId AND musicId = :musicId), :albumId, :artistId, :musicId)
    )"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":artistId"), artist_id);
    query.bindValue(qTEXT(":musicId"), music_id);

    DbIfFailedThrow1(query);    
}

void Database::removePlaylist(int32_t playlist_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM playlist WHERE playlistId=:playlistId"));
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    DbIfFailedThrow1(query);
}

void Database::removePlaylistAllMusic(int32_t playlist_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM playlistMusics WHERE playlistId=:playlistId"));
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    DbIfFailedThrow1(query);
    XAMP_LOG_D(logger_, "removePlaylistAllMusic playlist_id:{}", playlist_id);
}

void Database::updatePlaylistMusicChecked(int32_t playlist_music_id, bool is_checked) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE playlistMusics SET isChecked = :isChecked WHERE (playlistMusicsId = :playlistMusicsId)"));

    query.bindValue(qTEXT(":playlistMusicsId"), playlist_music_id);
    query.bindValue(qTEXT(":isChecked"), is_checked);

    DbIfFailedThrow1(query);
}

void Database::removePlaylistMusic(int32_t playlist_id, const QVector<int32_t>& select_music_ids) {
    SqlQuery query(db_);

    QString str = qTEXT("DELETE FROM playlistMusics WHERE playlistId=:playlistId AND musicId in (%0)");

    QStringList list;
    for (auto id : select_music_ids) {
        list << QString::number(id);
    }

    auto q = str.arg(list.join(qTEXT(",")));
    query.prepare(q);

    query.bindValue(qTEXT(":playlistId"), playlist_id);
    DbIfFailedThrow1(query);

    if (!query.next()) {
        XAMP_LOG_D(logger_, "RemovePlaylistMusic playlist:{} affected rows: 0 {}", playlist_id, q.toStdString());
    }
}
