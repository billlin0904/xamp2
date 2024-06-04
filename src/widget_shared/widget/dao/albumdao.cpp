#include <QDir>

#include <widget/database.h>
#include <widget/dao/playlistdao.h>
#include <widget/dao/musicdao.h>
#include <widget/dao/albumdao.h>

namespace dao {
    namespace {
        PlayListEntity fromSqlQuery(const SqlQuery& query) {
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

		query.prepare(qTEXT("UPDATE albums SET coverId = :coverId WHERE (albumId = :albumId)"));

		query.bindValue(qTEXT(":albumId"), album_id);
		query.bindValue(qTEXT(":coverId"), cover_id);

		DbIfFailedThrow1(query);
	}

	std::optional<AlbumStats> AlbumDao::getAlbumStats(int32_t album_id) const {
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

    int32_t AlbumDao::addOrUpdateAlbum(const QString& album, int32_t artist_id, int64_t album_time, uint32_t year, StoreType store_type, const QString& disc_id, bool is_hires) {
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

        query.bindValue(qTEXT(":album"), album);
        query.bindValue(qTEXT(":artistId"), artist_id);
        query.bindValue(qTEXT(":coverId"), getAlbumCoverId(album));
        query.bindValue(qTEXT(":storeType"), static_cast<int32_t>(store_type));
        query.bindValue(qTEXT(":dateTime"), album_time);
        query.bindValue(qTEXT(":discId"), disc_id);
        query.bindValue(qTEXT(":year"), year);
        query.bindValue(qTEXT(":isHiRes"), is_hires);

        DbIfFailedThrow1(query);

        const auto album_id = query.lastInsertId().toInt();
        return album_id;
    }

    void AlbumDao::updateAlbum(int32_t album_id, const QString& album) {
        SqlQuery query(db_);

        query.prepare(qTEXT("UPDATE albums SET album = :album WHERE (albumId = :albumId)"));

        query.bindValue(qTEXT(":albumId"), album_id);
        query.bindValue(qTEXT(":album"), album);

        DbIfFailedThrow1(query);
    }

    void AlbumDao::updateAlbumHeart(int32_t album_id, uint32_t heart) {
        SqlQuery query(db_);

        query.prepare(qTEXT("UPDATE albums SET heart = :heart WHERE (albumId = :albumId)"));

        query.bindValue(qTEXT(":albumId"), album_id);
        query.bindValue(qTEXT(":heart"), heart);

        DbIfFailedThrow1(query);
    }

    void AlbumDao::updateAlbumPlays(int32_t album_id) {
        SqlQuery query(db_);

        query.prepare(qTEXT("UPDATE albums SET plays = plays + 1 WHERE (albumId = :albumId)"));
        query.bindValue(qTEXT(":albumId"), album_id);
        DbIfFailedThrow1(query);
    }

    QString AlbumDao::getAlbumCoverId(int32_t album_id) const {
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

    int32_t AlbumDao::getAlbumId(const QString& album) const {
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

    QString AlbumDao::getAlbumCoverId(const QString& album) const {
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

    void AlbumDao::removeAlbumCategory(int32_t album_id) {
        SqlQuery query(db_);
        query.prepare(qTEXT("DELETE FROM albumCategories WHERE albumId=:albumId"));
        query.bindValue(qTEXT(":albumId"), album_id);
        DbIfFailedThrow1(query);
    }

    void AlbumDao::removeAlbumMusicAlbum(int32_t album_id) {
        SqlQuery query(db_);
        query.prepare(qTEXT("DELETE FROM albumMusic WHERE albumId=:albumId"));
        query.bindValue(qTEXT(":albumId"), album_id);
        DbIfFailedThrow1(query);
    }

    void AlbumDao::removeAlbum(int32_t album_id) {
        QList<PlayListEntity> entities;
        forEachAlbumMusic(album_id, [this, &entities](auto const& entity) {
            entities.push_back(entity);
            });
		dao::MusicDao music_dao(db_);
		dao::PlaylistDao playlist_dao(db_);
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
        query.prepare(qTEXT("DELETE FROM albums WHERE albumId=:albumId"));
        query.bindValue(qTEXT(":albumId"), album_id);
        DbIfFailedThrow1(query);
    }

    void AlbumDao::addAlbumCategory(int32_t album_id, const QString& category) const {
        SqlQuery query(db_);

        query.prepare(qTEXT(R"(
    INSERT INTO albumCategories (albumCategoryId, albumId, category)
    VALUES (NULL, :albumId, :category)
    )"));

        query.bindValue(qTEXT(":albumId"), album_id);
        query.bindValue(qTEXT(":category"), category);

        DbIfFailedThrow1(query);
    }

    void AlbumDao::addOrUpdateAlbumCategory(int32_t album_id, const QString& category) const {
        SqlQuery query(db_);

        query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO albumCategories (albumCategoryId, albumId, category)
    VALUES ((SELECT albumCategoryId FROM albumCategories WHERE albumId = :albumId), :albumId, :category)
    )"));

        query.bindValue(qTEXT(":albumId"), album_id);
        query.bindValue(qTEXT(":category"), category);

        DbIfFailedThrow1(query);
    }

    void AlbumDao::addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const {
        SqlQuery query(db_);

        query.prepare(qTEXT(R"(
    INSERT INTO albumArtist (albumArtistId, albumId, artistId)
    VALUES (NULL, :albumId, :artistId)
    )"));

        query.bindValue(qTEXT(":albumId"), album_id);
        query.bindValue(qTEXT(":artistId"), artist_id);

        DbIfFailedThrow1(query);
    }

    int32_t AlbumDao::getAlbumIdByDiscId(const QString& disc_id) const {
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

    void AlbumDao::updateAlbumByDiscId(const QString& disc_id, const QString& album) {
        SqlQuery query(db_);

        query.prepare(qTEXT("UPDATE albums SET album = :album WHERE (discId = :discId)"));

        query.bindValue(qTEXT(":album"), album);
        query.bindValue(qTEXT(":discId"), disc_id);

        DbIfFailedThrow1(query);
    }

    void AlbumDao::removeAlbumArtist(int32_t album_id) {
        SqlQuery query(db_);
        query.prepare(qTEXT("DELETE FROM albumArtist WHERE albumId=:albumId"));
        query.bindValue(qTEXT(":albumId"), album_id);
        DbIfFailedThrow1(query);
    }

    std::optional<QString> AlbumDao::getAlbumFirstMusicFilePath(int32_t album_id) const {
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

    void AlbumDao::removeAlbumMusic(int32_t album_id, int32_t music_id) {
        SqlQuery query(db_);
        query.prepare(qTEXT("DELETE FROM albumMusic WHERE albumId=:albumId AND musicId=:musicId"));
        query.bindValue(qTEXT(":albumId"), album_id);
        query.bindValue(qTEXT(":musicId"), music_id);
        DbIfFailedThrow1(query);
    }

    int32_t AlbumDao::getAlbumIdFromAlbumMusic(int32_t music_id) {
        SqlQuery query(db_);
        
        query.prepare(qTEXT("SELECT albumId FROM albumMusic WHERE musicId = (:musicId)"));
        query.bindValue(qTEXT(":musicId"), music_id);
        query.exec();
        
        if (query.next()) {
            return query.value(qTEXT("albumId")).toInt();
        }
        return kInvalidDatabaseId;
    }

    void AlbumDao::addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) {
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

    void AlbumDao::addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) {
      SqlQuery query(db_);

      query.prepare(qTEXT(R"(
      INSERT INTO albumArtist (albumArtistId, albumId, artistId)
      VALUES (NULL, :albumId, :artistId)
      )"));
      
      query.bindValue(qTEXT(":albumId"), album_id);
      query.bindValue(qTEXT(":artistId"), artist_id);
      
      DbIfFailedThrow1(query);
    }

    void AlbumDao::updateReplayGain(int32_t music_id,
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

    void AlbumDao::addOrUpdateTrackLoudness(int32_t album_id, int32_t artist_id, int32_t music_id, double track_loudness) {
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

    QStringList AlbumDao::getCategories() const {
      QStringList categories;
      SqlQuery query(db_);
      
      query.prepare(qTEXT("SELECT category FROM albumCategories GROUP BY category"));
      
      DbIfFailedThrow1(query);
      
      while (query.next()) {
          categories.append(query.value(qTEXT("category")).toString());
      }
      return categories;
    }

    QStringList AlbumDao::getYears() const {
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

    void AlbumDao::forEachAlbumMusic(int32_t album_id, std::function<void(PlayListEntity const&)>&& fun) {
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
            return;
        }
    
        while (query.next()) {
            fun(fromSqlQuery(query));
        }
    }

    void AlbumDao::updateAlbumSelectState(int32_t album_id, bool state) {
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

    QList<int32_t> AlbumDao::getSelectedAlbums() {
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

    void AlbumDao::forEachAlbum(std::function<void(int32_t)>&& fun) {
        SqlQuery query(db_);
        
        query.prepare(qTEXT("SELECT albumId FROM albums"));
        DbIfFailedThrow1(query);
        while (query.next()) {
            fun(query.value(qTEXT("albumId")).toInt());
        }
    }
}
