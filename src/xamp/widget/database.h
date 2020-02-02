//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlTableModel>
#include <QSqlRecord>

#include <QVariant>

#include <base/logger.h>
#include <base/exception.h>
#include <base/metadata.h>

#include "str_utilts.h"
#include "playlistentity.h"

class SqlException : public xamp::base::Exception {
public:
    explicit SqlException(QSqlError error);

    const char* what() const noexcept override;
};

#define IgnoreSqlError(expr) \
    try {\
    expr;\
    }\
    catch (const xamp::base::Exception& e) {\
    XAMP_LOG_DEBUG(e.what());\
    }

class Database {
public:
    static constexpr int32_t INVALID_DATABASE_ID = -1;

    static Database& Instance() {
        static Database instance;
        return instance;
    }

    ~Database();

    void open(const QString& file_name);

    int32_t addTable(const QString& name, int32_t table_index, int32_t playlist_id);

    int32_t addPlaylist(const QString& name, int32_t playlistIndex);

    void setAlbumCover(int32_t album_id, const QString& album, const QString& cover_id);

    void addTablePlaylist(int32_t tableId, int32_t playlist_id);

    int32_t addOrUpdateMusic(const xamp::base::Metadata& medata, int32_t playlist_id);

    int32_t addOrUpdateArtist(const QString& artist);

    void updateDiscogsArtistId(int32_t artist_id, const QString& discogs_artist_id);

    void updateArtistCoverId(int32_t artist_id, const QString& coverId);

    void updateArtistMbid(int32_t artist_id, const QString& mbid);

    int32_t addOrUpdateAlbum(const QString& album, int32_t artist_id);

    void addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const;

    void addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id);

    QString getAlbumCoverId(int32_t album_id) const;

    void setTableName(int32_t table_id, const QString &name);

    template <typename Function>
    void forEachTable(Function &&callback) {
        QSqlTableModel model(nullptr, db_);

        model.setTable(Q_UTF8("tables"));
        model.setSort(1, Qt::AscendingOrder);
        model.select();

        for (auto i = 0; i < model.rowCount(); ++i) {
            auto record = model.record(i);
            callback(record.value(Q_UTF8("tableId")).toInt(),
                     record.value(Q_UTF8("tableIndex")).toInt(),
                     record.value(Q_UTF8("playlistId")).toInt(),
                     record.value(Q_UTF8("name")).toString());
        }
    }

    template <typename Function>
    void forEachPlaylistMusic(int32_t playlist_id, Function &&fun) {
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
            entity.track = query.value(Q_UTF8("track")).toInt();
            entity.title = query.value(Q_UTF8("title")).toString();
            entity.file_name = query.value(Q_UTF8("fileName")).toString();
            entity.album = query.value(Q_UTF8("album")).toString();
            entity.artist = query.value(Q_UTF8("artist")).toString();
            entity.file_ext = query.value(Q_UTF8("fileExt")).toString();
            entity.parent_path = query.value(Q_UTF8("parentPath")).toString();
            entity.duration = query.value(Q_UTF8("duration")).toDouble();
            entity.bitrate = query.value(Q_UTF8("bitrate")).toInt();
            entity.samplerate = query.value(Q_UTF8("samplerate")).toInt();
            entity.cover_id = query.value(Q_UTF8("coverId")).toString();
            fun(entity);
        }
    }

    void removePlaylistMusic(int32_t playlist_id, const QVector<int>& select_music_ids);

    int32_t findTablePlaylistId(int32_t table_id) const;

    bool isPlaylistExist(int32_t playlist_id) const;

    void addMusicToPlaylist(int32_t music_id, int32_t playlist_id) const;
private:
    Database();

    void addAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const;

    void createTableIfNotExist();

    QSqlDatabase db_;
};

