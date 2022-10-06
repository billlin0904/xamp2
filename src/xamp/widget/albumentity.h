//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>

#include <QString>
#include <QVariant>
#include <QModelIndex>

#include <widget/playlistentity.h>

struct AlbumEntity final {
    int32_t album_id{ 0 };
    int32_t artist_id{ 0 };
    int32_t music_id{ 0 };
    double album_replay_gain{ 0 };
    double track_replay_gain{ 0 };
    QString album;
    QString title;
    QString artist;
    QString file_path;
    QString file_ext;
    QString cover_id;
};

// for QVariantÂà´«¨Ï¥Î
Q_DECLARE_METATYPE(AlbumEntity)

QVariant getIndexValue(const QModelIndex& index, int i);
AlbumEntity getAlbumEntity(const QModelIndex& index);
AlbumEntity toAlbumEntity(const PlayListEntity& item);

