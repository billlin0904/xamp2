//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
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

class Database final {
public:
    static constexpr int32_t INVALID_DATABASE_ID = -1;

    friend class Singleton<Database>;

    ~Database();

    void open(const QString& file_name);

    void flush();

    int32_t AddTable(const QString& name, int32_t table_index, int32_t playlist_id);

    int32_t AddPlaylist(const QString& name, int32_t playlistIndex);

    void SetAlbumCover(int32_t album_id, const QString& album, const QString& cover_id);

    std::optional<AlbumStats> GetAlbumStats(int32_t album_id) const;

    void AddTablePlaylist(int32_t tableId, int32_t playlist_id);

    int32_t AddOrUpdateMusic(const Metadata& medata, int32_t playlist_id);

    int32_t AddOrUpdateArtist(const QString& artist);

    void UpdateDiscogsArtistId(int32_t artist_id, const QString& discogs_artist_id);

    void UpdateArtistCoverId(int32_t artist_id, const QString& coverId);

    void UpdateArtistMbid(int32_t artist_id, const QString& mbid);

    void UpdateMusicFingerprint(int32_t music_id, const QString& fingerprint);

    void UpdateMusicFilePath(int32_t music_id, const QString& file_path);

    void UpdateMusicRating(int32_t music_id, int32_t rating);

    int32_t AddOrUpdateAlbum(const QString& album, int32_t artist_id);

    void AddOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const;

    void AddOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id);

    static void AddPlaybackHistory(int32_t album_id, int32_t artist_id, int32_t music_id);

    void DeleteOldestHistory();

    QString GetAlbumCoverId(int32_t album_id) const;

    QString GetArtistCoverId(int32_t artist_id) const;

    void SetTableName(int32_t table_id, const QString &name);

    void RemoveAlbum(int32_t album_id);

    template <typename Function>
    void ForEachTable(Function &&fun) {
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
    void ForEachAlbumMusic(int32_t album_id, Function &&fun) {
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
            fun(entity);
        }
    }

    template <typename Function>
    void ForEachPlaylistMusic(int32_t playlist_id, Function &&fun) {
        QSqlQuery query;

        query.prepare(Q_UTF8(R"(
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
                             )"));

        query.bindValue(Q_UTF8(":playlist_id"), playlist_id);

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
            fun(entity);
        }
    }

    void RemoveAllArtist();

    void RemoveArtistId(int32_t artist_id);

    void RemoveMusic(int32_t music_id);

    void RemovePlaylistAllMusic(int32_t playlist_id);
	
    void RemovePlaylistMusic(int32_t playlist_id, const QVector<int>& select_music_ids);

    int32_t FindTablePlaylistId(int32_t table_id) const;

    bool IsPlaylistExist(int32_t playlist_id) const;

    void AddMusicToPlaylist(int32_t music_id, int32_t playlist_id) const;

    void SetNowPlaying(int32_t playlist_id, int32_t music_id);

    void ClearNowPlaying(int32_t playlist_id);
private:
    Database();

    void RemoveAlbumArtist(int32_t album_id);

    void RemovePlaybackHistory(int32_t music_id);

    void RemoveAlbumMusicId(int32_t music_id);

    void RemovePlaylistMusics(int32_t music_id);

    void RemoveAlbumArtistId(int32_t artist_id);

    void AddAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const;

    void CreateTableIfNotExist();

    QSqlDatabase db_;
};

