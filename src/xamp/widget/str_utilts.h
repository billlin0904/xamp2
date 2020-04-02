//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QColor>
#include <QString>
#include <QVariant>
#include <QModelIndex>

#define Q_UTF8(str) QLatin1String{str}
#define Q_EMPTY_STR QLatin1String{""}

struct MusicEntity {
    int32_t album_id{ 0 };
    int32_t artist_id{ 0 };
    int32_t music_id{ 0 };
    QString album;
    QString title;
    QString artist;
    QString file_path;
    QString file_ext;
    QString cover_id;
};

Q_DECLARE_METATYPE(MusicEntity)

inline QVariant getIndexValue(const QModelIndex& index, int i) {
    return index.model()->data(index.model()->index(index.row(), i));
}

inline MusicEntity getAlbumEntity(const QModelIndex& index) {
    auto title = getIndexValue(index, 1).toString();
    auto musicId = getIndexValue(index, 3).toInt();
    auto artist = getIndexValue(index, 4).toString();
    auto file_ext = getIndexValue(index, 5).toString();
    auto file_path = getIndexValue(index, 6).toString();
    auto cover_id = getIndexValue(index, 7).toString();
    auto album = getIndexValue(index, 8).toString();

    auto artistId = getIndexValue(index, 9).toInt();
    auto albumId = getIndexValue(index, 10).toInt();

    MusicEntity entity;

    entity.music_id = musicId;
    entity.album = album;
    entity.title = title;
    entity.artist = artist;
    entity.cover_id = cover_id;
    entity.file_path = file_path;
    entity.file_ext = file_ext;
    entity.album_id = albumId;
    entity.artist_id = artistId;

    return entity;
}

inline QString formatBytes(size_t bytes) noexcept {
    constexpr float tb = 1099511627776;
    constexpr float gb = 1073741824;
    constexpr float mb = 1048576;
    constexpr float kb = 1024;

    QString result;

    if (bytes >= tb)
        result.sprintf("%.2f TB", (float)bytes / tb);
    else if (bytes >= gb && bytes < tb)
        result.sprintf("%.2f GB", (float)bytes / gb);
    else if (bytes >= mb && bytes < gb)
        result.sprintf("%.2f MB", (float)bytes / mb);
    else if (bytes >= kb && bytes < mb)
        result.sprintf("%.2f KB", (float)bytes / kb);
    else if (bytes < kb)
        result.sprintf("%.2f Bytes", (float)bytes);
    else
        result.sprintf("%.2f Bytes", (float)bytes);

    return result;
}

inline QString colorToString(QColor color) noexcept {
    return QString().sprintf("rgba(%d,%d,%d,%d)", 
        color.red(), color.green(), color.blue(), color.alpha());
}


inline QString backgroundColorToString(QColor color) {
    return Q_UTF8("background-color: ") + colorToString(color) + Q_UTF8(";");
}
