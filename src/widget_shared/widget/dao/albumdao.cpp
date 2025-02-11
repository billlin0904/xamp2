#include <QDir>

#include <widget/database.h>
#include <widget/dao/playlistdao.h>
#include <widget/dao/musicdao.h>
#include <widget/dao/albumdao.h>

#include <base/rng.h>

namespace dao {
    namespace {
        PlayListEntity fromSqlQuery(const SqlQuery& query) {
            PlayListEntity entity;
            entity.album_id = query.value("albumId"_str).toInt();
            entity.artist_id = query.value("artistId"_str).toInt();
            entity.music_id = query.value("musicId"_str).toInt();
            entity.file_path = query.value("path"_str).toString();
            entity.track = query.value("track"_str).toUInt();
            entity.title = query.value("title"_str).toString();
            entity.file_name = query.value("fileName"_str).toString();
            entity.album = query.value("album"_str).toString();
            entity.artist = query.value("artist"_str).toString();
            entity.file_extension = query.value("fileExt"_str).toString();
            entity.parent_path = query.value("parentPath"_str).toString();
            entity.duration = query.value("duration"_str).toDouble();
            entity.bit_rate = query.value("bitRate"_str).toUInt();
            entity.sample_rate = query.value("sampleRate"_str).toUInt();
            entity.cover_id = query.value("coverId"_str).toString();
            entity.rating = query.value("rating"_str).toUInt();
            entity.album_replay_gain = query.value("albumReplayGain"_str).toDouble();
            entity.album_peak = query.value("albumPeak"_str).toDouble();
            entity.track_replay_gain = query.value("trackReplayGain"_str).toDouble();
            entity.track_peak = query.value("trackPeak"_str).toDouble();
            entity.track_loudness = query.value("trackLoudness"_str).toDouble();

            entity.genre = query.value("genre"_str).toString();
            entity.comment = query.value("comment"_str).toString();
            entity.file_size = query.value("fileSize"_str).toULongLong();

            entity.lyrc = query.value("lyrc"_str).toString();
            entity.trlyrc = query.value("trLyrc"_str).toString();

            QFileInfo file_info(entity.file_path);
            entity.file_extension = file_info.suffix();
            entity.file_name = file_info.completeBaseName();
            entity.parent_path = toNativeSeparators(file_info.dir().path());

            return entity;
        }
    }

    AlbumDao::AlbumDao()
        : AlbumDao(qGuiDb.getDatabase()) {
    }

	AlbumDao::AlbumDao(QSqlDatabase& db)
		: db_(db) {
	}

	void AlbumDao::setAlbumCover(int32_t album_id, const QString& cover_id) {
		XAMP_ENSURES(!cover_id.isEmpty());

		SqlQuery query(db_);

		query.prepare("UPDATE albums SET coverId = :coverId WHERE (albumId = :albumId)"_str);

		query.bindValue(":albumId"_str, album_id);
		query.bindValue(":coverId"_str, cover_id);

		DbIfFailedThrow1(query);
	}

	std::optional<AlbumStats> AlbumDao::getAlbumStats(int32_t album_id) const {
        SqlQuery query(db_);

        query.prepare(R"(
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
        albums.albumId = :albumId;)"_str);

        query.bindValue(":albumId"_str, album_id);

        DbIfFailedThrow1(query);

        while (query.next()) {
            AlbumStats stats;
            stats.songs = query.value("tracks"_str).toInt();
            stats.year = query.value("year"_str).toInt();
            stats.durations = query.value("durations"_str).toDouble();
            stats.file_size = query.value("fileSize"_str).toULongLong();
            return MakeOptional<AlbumStats>(std::move(stats));
        }

        return std::nullopt;
	}

    int32_t AlbumDao::addOrUpdateAlbum(const QString& album,
        int32_t artist_id,
        int64_t album_time,
        uint32_t year,
        StoreType store_type,
        const QString& disc_id,
        bool is_hires) {
        XAMP_ENSURES(!album.isEmpty());
        XAMP_ENSURES(year != UINT32_MAX);

        SqlQuery query(db_);

        query.prepare(R"(
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
    )"_str);

        query.bindValue(":album"_str, album);
        query.bindValue(":artistId"_str, artist_id);
        query.bindValue(":coverId"_str, getAlbumCoverId(album));
        query.bindValue(":storeType"_str, static_cast<int32_t>(store_type));
        query.bindValue(":dateTime"_str, album_time);
        query.bindValue(":discId"_str, disc_id);
        query.bindValue(":year"_str, year);
        query.bindValue(":isHiRes"_str, is_hires);

        DbIfFailedThrow1(query);

        const auto album_id = query.lastInsertId().toInt();
        return album_id;
    }

    void AlbumDao::updateAlbum(int32_t album_id, const QString& album) {
        SqlQuery query(db_);

        query.prepare("UPDATE albums SET album = :album WHERE (albumId = :albumId)"_str);

        query.bindValue(":albumId"_str, album_id);
        query.bindValue(":album"_str, album);

        DbIfFailedThrow1(query);
    }

    void AlbumDao::updateAlbumHeart(int32_t album_id, uint32_t heart) {
        SqlQuery query(db_);

        query.prepare("UPDATE albums SET heart = :heart WHERE (albumId = :albumId)"_str);

        query.bindValue(":albumId"_str, album_id);
        query.bindValue(":heart"_str, heart);

        DbIfFailedThrow1(query);
    }

    void AlbumDao::updateAlbumPlays(int32_t album_id) {
        SqlQuery query(db_);

        query.prepare("UPDATE albums SET plays = plays + 1 WHERE (albumId = :albumId)"_str);
        query.bindValue(":albumId"_str, album_id);
        DbIfFailedThrow1(query);
    }

    QString AlbumDao::getAlbumCoverId(int32_t album_id) const {
        SqlQuery query(db_);

        query.prepare("SELECT coverId FROM albums WHERE albumId = (:albumId)"_str);
        query.bindValue(":albumId"_str, album_id);

        DbIfFailedThrow1(query);

        const auto index = query.record().indexOf("coverId"_str);
        if (query.next()) {
            return query.value(index).toString();
        }
        return kEmptyString;
    }

    int32_t AlbumDao::getAlbumId(const QString& album) const {
        SqlQuery query(db_);

        query.prepare("SELECT albumId FROM albums WHERE album = (:album)"_str);
        query.bindValue(":album"_str, album);

        DbIfFailedThrow1(query);

        const auto index = query.record().indexOf("albumId"_str);
        if (query.next()) {
            return query.value(index).toInt();
        }
        return kInvalidDatabaseId;
    }

    QString AlbumDao::getAlbumCoverId(const QString& album) const {
        SqlQuery query(db_);

        query.prepare("SELECT coverId FROM albums WHERE album = (:album)"_str);
        query.bindValue(":album"_str, album);

        DbIfFailedThrow1(query);

        const auto index = query.record().indexOf("coverId"_str);
        if (query.next()) {
            return query.value(index).toString();
        }
        return kEmptyString;
    }

    void AlbumDao::removeAlbumCategory(int32_t album_id) {
        SqlQuery query(db_);
        query.prepare("DELETE FROM albumCategories WHERE albumId=:albumId"_str);
        query.bindValue(":albumId"_str, album_id);
        DbIfFailedThrow1(query);
    }

    void AlbumDao::removeAlbumMusicAlbum(int32_t album_id) {
        SqlQuery query(db_);
        query.prepare("DELETE FROM albumMusic WHERE albumId=:albumId"_str);
        query.bindValue(":albumId"_str, album_id);
        DbIfFailedThrow1(query);
    }

    int32_t AlbumDao::getRandomMusicId(int32_t album_id, PRNG& rng) {
        SqlQuery query(db_);
        query.prepare("SELECT musicId FROM albumMusic WHERE albumId = :album_id"_str);
        query.bindValue(":album_id"_str, album_id);

        DbIfFailedThrow1(query);

        QVector<int32_t> song_ids;
        while (query.next()) {
            song_ids.append(query.value(0).toInt());
        }

        if (song_ids.isEmpty()) {
            return kInvalidDatabaseId;
        }

        int32_t random_index = rng.NextInt32(0, song_ids.size() - 1);
        return song_ids[random_index];
    }

    int32_t AlbumDao::getRandomAlbumId(int32_t album_id, PRNG& rng) {
        SqlQuery query(db_);
        query.prepare("SELECT albumId FROM albums WHERE albumId != :album_id"_str);
        query.bindValue(":album_id"_str, album_id);

        DbIfFailedThrow1(query);

        QVector<int32_t> album_ids;
        while (query.next()) {
            album_ids.append(query.value(0).toInt());
        }

        if (album_ids.isEmpty()) {
            return kInvalidDatabaseId;
        }

        int32_t random_index = rng.NextInt32(0, album_ids.size() - 1);
        return album_ids[random_index];
    }

    void AlbumDao::removeAlbum(int32_t album_id) {
        QList<PlayListEntity> entities;
        forEachAlbumMusic(album_id, [this, &entities](auto const& entity) {
            entities.push_back(entity);
            });
		MusicDao music_dao(db_);
		PlaylistDao playlist_dao(db_);
        if (!entities.empty()) {
            Q_FOREACH(const auto & entity, entities) {
                QList<int32_t> playlist_ids;
                playlist_dao.forEachPlaylist([&playlist_ids, this](auto playlistId, auto, auto, auto, auto) {
                    playlist_ids.push_back(playlistId);
                    });

                Q_FOREACH(auto playlistId, playlist_ids) {
                    playlist_dao.removePlaylistMusic(playlistId, QVector<int32_t>{ entity.music_id });
                }
                removeAlbumCategory(album_id);
                removeAlbumMusicAlbum(album_id);
                music_dao.removeTrackLoudnessMusicId(entity.music_id);
                music_dao.removeMusic(entity.music_id);
            }
        }
        else {
            removeAlbumCategory(album_id);
            removeAlbumMusicAlbum(album_id);
        }

        SqlQuery query(db_);
        query.prepare("DELETE FROM albums WHERE albumId=:albumId"_str);
        query.bindValue(":albumId"_str, album_id);
        DbIfFailedThrow1(query);
    }

    void AlbumDao::addAlbumCategory(int32_t album_id, const QString& category) const {
        SqlQuery query(db_);

        query.prepare(R"(
    INSERT INTO albumCategories (albumCategoryId, albumId, category)
    VALUES (NULL, :albumId, :category)
    )"_str);

        query.bindValue(":albumId"_str, album_id);
        query.bindValue(":category"_str, category);

        DbIfFailedThrow1(query);
    }

    void AlbumDao::addOrUpdateAlbumCategory(int32_t album_id, const QString& category) const {
        SqlQuery query(db_);

        query.prepare(R"(
    INSERT OR REPLACE INTO albumCategories (albumCategoryId, albumId, category)
    VALUES ((SELECT albumCategoryId FROM albumCategories WHERE albumId = :albumId), :albumId, :category)
    )"_str);

        query.bindValue(":albumId"_str, album_id);
        query.bindValue(":category"_str, category);

        DbIfFailedThrow1(query);
    }

    void AlbumDao::addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const {
        SqlQuery query(db_);

        query.prepare(R"(
    INSERT INTO albumArtist (albumArtistId, albumId, artistId)
    VALUES (NULL, :albumId, :artistId)
    )"_str);

        query.bindValue(":albumId"_str, album_id);
        query.bindValue(":artistId"_str, artist_id);

        DbIfFailedThrow1(query);
    }

    int32_t AlbumDao::getAlbumIdByDiscId(const QString& disc_id) const {
        SqlQuery query(db_);
        query.prepare("SELECT albumId FROM albums WHERE discId = (:discId)"_str);
        query.bindValue(":discId"_str, disc_id);
        DbIfFailedThrow1(query);

        const auto index = query.record().indexOf("albumId"_str);
        if (query.next()) {
            return query.value(index).toInt();
        }
        return kInvalidDatabaseId;
    }

    void AlbumDao::updateAlbumByDiscId(const QString& disc_id, const QString& album) {
        SqlQuery query(db_);

        query.prepare("UPDATE albums SET album = :album WHERE (discId = :discId)"_str);

        query.bindValue(":album"_str, album);
        query.bindValue(":discId"_str, disc_id);

        DbIfFailedThrow1(query);
    }

    void AlbumDao::removeAlbumArtist(int32_t album_id) {
        SqlQuery query(db_);
        query.prepare("DELETE FROM albumArtist WHERE albumId=:albumId"_str);
        query.bindValue(":albumId"_str, album_id);
        DbIfFailedThrow1(query);
    }

    std::optional<QString> AlbumDao::getAlbumFirstMusicFilePath(int32_t album_id) const {
        SqlQuery query(R"(
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
)"_str, db_);
        query.addBindValue(album_id);
        query.exec();
        while (query.next()) {
            return MakeOptional<QString>(query.value("path"_str).toString());
        }
        return std::nullopt;
    }

    void AlbumDao::removeAlbumMusic(int32_t album_id, int32_t music_id) {
        SqlQuery query(db_);
        query.prepare("DELETE FROM albumMusic WHERE albumId=:albumId AND musicId=:musicId"_str);
        query.bindValue(":albumId"_str, album_id);
        query.bindValue(":musicId"_str, music_id);
        DbIfFailedThrow1(query);
    }

    int32_t AlbumDao::getAlbumIdFromAlbumMusic(int32_t music_id) {
        SqlQuery query(db_);
        
        query.prepare("SELECT albumId FROM albumMusic WHERE musicId = (:musicId)"_str);
        query.bindValue(":musicId"_str, music_id);
        query.exec();
        
        if (query.next()) {
            return query.value("albumId"_str).toInt();
        }
        return kInvalidDatabaseId;
    }

    void AlbumDao::addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) {
      XAMP_EXPECTS(album_id != kInvalidDatabaseId);
      XAMP_EXPECTS(artist_id != kInvalidDatabaseId);
      XAMP_EXPECTS(music_id != kInvalidDatabaseId);
      
      SqlQuery query(db_);
      
      query.prepare(R"(
      INSERT OR REPLACE INTO albumMusic (albumMusicId, albumId, artistId, musicId)
      VALUES ((SELECT albumMusicId from albumMusic where albumId = :albumId AND artistId = :artistId AND musicId = :musicId), :albumId, :artistId, :musicId)
      )"_str);
      
      query.bindValue(":albumId"_str, album_id);
      query.bindValue(":artistId"_str, artist_id);
      query.bindValue(":musicId"_str, music_id);
      
      DbIfFailedThrow1(query);    
    }

    void AlbumDao::addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) {
      SqlQuery query(db_);

      query.prepare(R"(
      INSERT INTO albumArtist (albumArtistId, albumId, artistId)
      VALUES (NULL, :albumId, :artistId)
      )"_str);
      
      query.bindValue(":albumId"_str, album_id);
      query.bindValue(":artistId"_str, artist_id);
      
      DbIfFailedThrow1(query);
    }

    void AlbumDao::updateReplayGain(int32_t music_id,
        double album_rg_gain,
        double album_peak,
        double track_rg_gain,
        double track_peak) {
        SqlQuery query(db_);
    
        query.prepare("UPDATE musics SET albumReplayGain = :albumReplayGain, albumPeak = :albumPeak, trackReplayGain = :trackReplayGain, trackPeak = :trackPeak WHERE (musicId = :musicId)"_str);
    
        query.bindValue(":musicId"_str, music_id);
        query.bindValue(":albumReplayGain"_str, album_rg_gain);
        query.bindValue(":albumPeak"_str, album_peak);
        query.bindValue(":trackReplayGain"_str, track_rg_gain);
        query.bindValue(":trackPeak"_str, track_peak);
    
        DbIfFailedThrow1(query);
    }

    void AlbumDao::addOrUpdateTrackLoudness(int32_t album_id, int32_t artist_id, int32_t music_id, double track_loudness) {
        SqlQuery query(db_);
    
        query.prepare(R"(
        INSERT OR REPLACE INTO musicLoudness (musicLoudnessId, albumId, artistId, musicId, trackLoudness)
        VALUES
        (
        (SELECT musicLoudnessId FROM musicLoudness WHERE albumId = :albumId AND artistId = :artistId AND musicId = :musicId AND trackLoudness = :trackLoudness),
        :albumId,
        :artistId,
        :musicId,
        :trackLoudness
        )
        )"_str);
    
        query.bindValue(":albumId"_str, album_id);
        query.bindValue(":artistId"_str, artist_id);
        query.bindValue(":musicId"_str, music_id);
        query.bindValue(":trackLoudness"_str, track_loudness);
    
        DbIfFailedThrow1(query);
    }

    QStringList AlbumDao::getCategories() const {
      QStringList categories;
      SqlQuery query(db_);
      
      query.prepare("SELECT category FROM albumCategories GROUP BY category"_str);
      
      DbIfFailedThrow1(query);
      
      while (query.next()) {
          categories.append(query.value("category"_str).toString());
      }
      return categories;
    }

    QStringList AlbumDao::getYears() const {
        QStringList years;
    
        SqlQuery query(db_);
        query.prepare(R"(
    SELECT
        year AS group_year,
        COUNT(*) AS count
    FROM albums
    WHERE group_year != 0
    GROUP BY group_year
    ORDER BY group_year DESC;
        )"_str
        );
    
        DbIfFailedThrow1(query);
    
        while (query.next()) {
            auto genre = query.value("group_year"_str).toString();
            if (genre.isEmpty()) {
                continue;
            }
            years.append(genre);
        }
        return years;
    }

    void AlbumDao::forEachAlbumMusic(int32_t album_id, std::function<void(PlayListEntity const&)>&& fun) {
        SqlQuery query(R"(
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
    )"_str, db_);
        query.addBindValue(album_id);
    
        if (!query.exec()) {
            return;
        }
    
        while (query.next()) {
            fun(fromSqlQuery(query));
        }
    }

    void AlbumDao::updateAlbumSelectState(int32_t album_id, bool state) {
        SqlQuery query(db_);
        
        if (album_id != kInvalidDatabaseId) {
            query.prepare("UPDATE albums SET isSelected = :isSelected WHERE (albumId = :albumId) AND storeType == 1"_str);
            query.bindValue(":albumId"_str, album_id);
        }
        else {
            query.prepare("UPDATE albums SET isSelected = :isSelected WHERE storeType == 1"_str);
        }
        
        query.bindValue(":isSelected"_str, state ? 1: 0);
        
        DbIfFailedThrow1(query);
    }

    QList<int32_t> AlbumDao::getSelectedAlbums() {
        QList<int32_t> selected_albums;
        SqlQuery query(db_);
        
        query.prepare("SELECT albumId FROM albums WHERE isSelected = 1"_str);
        
        DbIfFailedThrow1(query);
        
        const auto index = query.record().indexOf("albumId"_str);
        while (query.next()) {
            selected_albums.append(query.value(index).toInt());
        }
        return selected_albums;
    }

    void AlbumDao::forEachAlbumCover(std::function<void(const QString&)>&& fun, int32_t limit) {
        SqlQuery query(db_);

        query.prepare(qFormat("SELECT coverId FROM albums LIMIT %1").arg(limit));
        DbIfFailedThrow1(query);
        while (query.next()) {
            fun(query.value("coverId"_str).toString());
        }
    }

    void AlbumDao::forEachAlbum(std::function<void(int32_t)>&& fun) {
        SqlQuery query(db_);
        
        query.prepare("SELECT albumId FROM albums"_str);
        DbIfFailedThrow1(query);
        while (query.next()) {
            fun(query.value("albumId"_str).toInt());
        }
    }
}
