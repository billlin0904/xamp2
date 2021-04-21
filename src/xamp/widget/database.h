//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlTableModel>
#include <QSqlRecord>

#include <QVariant>

#include <widget/widget_shared.h>

#include <widget/str_utilts.h>
#include <widget/playlistentity.h>

class SqlException final : public Exception {
public:
    explicit SqlException(QSqlError error);

    virtual const char* what() const noexcept override;
};

#define IgnoreSqlError(expr) \
    try {\
    expr;\
    }\
    catch (const Exception& e) {\
    XAMP_LOG_DEBUG(e.what());\
    }

struct AlbumStats {
    int32_t tracks{ 0 };
    double durations{ 0 };
};

struct ArtistStats {
    int32_t albums{ 0 };
    int32_t tracks{ 0 };
    double durations{ 0 };
};

class Database final {
public:
    static constexpr int32_t kInvalidId = -1;

    friend class Singleton<Database>;

    ~Database();

    void open(const QString& file_name);

    void flush();

    int32_t addTable(const QString& name, int32_t table_index, int32_t playlist_id);

    int32_t addPlaylist(const QString& name, int32_t playlistIndex);

    void addDevice(const QString& deviceId, const QString& deviceTypeId, const QString& name, const QStringList & supportSampleRates, bool is_support_dsd);

    void setAlbumCover(int32_t album_id, const QString& album, const QString& cover_id);

    std::optional<AlbumStats> getAlbumStats(int32_t album_id) const;

    std::optional<ArtistStats> getArtistStats(int32_t artist_id) const;

    void addTablePlaylist(int32_t tableId, int32_t playlist_id);

    int32_t addOrUpdateMusic(const Metadata& medata, int32_t playlist_id);

    int32_t addOrUpdateArtist(const QString& artist);

    void updateDiscogsArtistId(int32_t artist_id, const QString& discogs_artist_id);

    void updateArtistCoverId(int32_t artist_id, const QString& coverId);

    void updateArtistMbid(int32_t artist_id, const QString& mbid);

    void updateMusicFingerprint(int32_t music_id, const QString& fingerprint);

    void updateMusicFilePath(int32_t music_id, const QString& file_path);

    void updateMusicRating(int32_t music_id, int32_t rating);

    void updateLUFS(int32_t music_id, double lufs, double true_peak);

    int32_t addOrUpdateAlbum(const QString& album, int32_t artist_id);

    void addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const;

    void addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id);

    void addPlaybackHistory(int32_t album_id, int32_t artist_id, int32_t music_id);

    void deleteOldestHistory();

    QString getAlbumCoverId(int32_t album_id) const;

    QString getArtistCoverId(int32_t artist_id) const;

    void setTableName(int32_t table_id, const QString &name);

    void removeAlbum(int32_t album_id);

    template <typename Function>
    void forEachTable(Function &&fun) {
        QSqlTableModel model(nullptr, db_);

        model.setTable(Q_UTF8("tables"));
        model.setSort(1, Qt::AscendingOrder);
        model.select();

        for (auto i = 0; i < model.rowCount(); ++i) {
            auto record = model.record(i);
            fun(record.value(Q_UTF8("tableId")).toInt(),
                record.value(Q_UTF8("tableIndex")).toInt(),
                record.value(Q_UTF8("playlistId")).toInt(),
                record.value(Q_UTF8("name")).toString());
        }
    }

    template <typename Function>
    void forEachAlbumMusic(int32_t album_id, Function &&fun) {
        QSqlQuery query(Q_UTF8(R"(
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
    albums.albumId = ?;)"));
        query.addBindValue(album_id);

        if (!query.exec()) {
            XAMP_LOG_DEBUG("{}", query.lastError().text().toStdString());
        }

        while (query.next()) {
            PlayListEntity entity;
            entity.album_id = query.value(Q_UTF8("albumId")).toInt();
            entity.artist_id = query.value(Q_UTF8("artistId")).toInt();
            entity.music_id = query.value(Q_UTF8("musicId")).toInt();
            entity.file_path = query.value(Q_UTF8("path")).toString();
            entity.track = query.value(Q_UTF8("track")).toUInt();
            entity.title = query.value(Q_UTF8("title")).toString();
            entity.file_name = query.value(Q_UTF8("fileName")).toString();
            entity.album = query.value(Q_UTF8("album")).toString();
            entity.artist = query.value(Q_UTF8("artist")).toString();
            entity.file_ext = query.value(Q_UTF8("fileExt")).toString();
            entity.parent_path = query.value(Q_UTF8("parentPath")).toString();
            entity.duration = query.value(Q_UTF8("duration")).toDouble();
            entity.bitrate = query.value(Q_UTF8("bitrate")).toUInt();
            entity.samplerate = query.value(Q_UTF8("samplerate")).toUInt();
            entity.cover_id = query.value(Q_UTF8("coverId")).toString();
            entity.fingerprint = query.value(Q_UTF8("fingerprint")).toString();
            entity.rating = query.value(Q_UTF8("rating")).toUInt();
            entity.lufs = query.value(Q_UTF8("lufs")).toDouble();
            fun(entity);
        }
    }

    void removeAllArtist();

    void removeArtistId(int32_t artist_id);

    void removeMusic(int32_t music_id);

    void removeMusic(QString const& file_path);

    void removePlaylistAllMusic(int32_t playlist_id);
	
    void removePlaylistMusic(int32_t playlist_id, const QVector<int>& select_music_ids);

    int32_t findTablePlaylistId(int32_t table_id) const;

    bool isPlaylistExist(int32_t playlist_id) const;

    void addMusicToPlaylist(int32_t music_id, int32_t playlist_id) const;

    void setNowPlaying(int32_t playlist_id, int32_t music_id);

    void clearNowPlaying(int32_t playlist_id);
private:
    Database();

    void removeAlbumArtist(int32_t album_id);

    void removePlaybackHistory(int32_t music_id);

    void removeAlbumMusicId(int32_t music_id);

    void removePlaylistMusics(int32_t music_id);

    void removeAlbumArtistId(int32_t artist_id);

    void addAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const;

    void createTableIfNotExist();

    QSqlDatabase db_;
};

