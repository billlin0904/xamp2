//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#include <QString>
#include <QIcon>

struct PlayListEntity {
    int32_t music_id{};
    int32_t album_id{};
    int32_t artist_id{};
    int32_t track{};
    int32_t bitrate{};
    int32_t samplerate{};
    double duration{};
    QIcon playing_ico;
    QString file_path;
    QString title;
    QString album;
    QString artist;
    QString file_ext;
    QString cover_id;
    QString file_name;
};

Q_DECLARE_METATYPE(PlayListEntity)

