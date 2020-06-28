//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#include <QString>
#include <QIcon>

#include <widget/starrating.h>

struct PlayListEntity {
    int32_t music_id{0};
    int32_t album_id{0};
    int32_t artist_id{0};
    uint32_t track{0};
    uint32_t bitrate{0};
    uint32_t samplerate{0};
    uint32_t rating{0};
    double duration{0};
    QString file_path;
    QString parent_path;
    QString title;
    QString album;
    QString artist;
    QString file_ext;
    QString cover_id;
    QString file_name;
    QString fingerprint;
};

Q_DECLARE_METATYPE(PlayListEntity)

