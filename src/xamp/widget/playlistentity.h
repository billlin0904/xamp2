//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#include <QString>
#include <QIcon>
#include <QUrl>

#include <widget/starrating.h>

struct PlayListEntity final {
    bool playing{false};
    int32_t music_id{0};
    int32_t album_id{0};
    int32_t artist_id{0};
    uint32_t track{0};
    uint32_t bitrate{0};
    uint32_t samplerate{0};
    uint32_t rating{0};
    double duration{0};
    double lufs{0};
    double true_peak{ 0 };
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

// for QVariantÂà´«¨Ï¥Î
Q_DECLARE_METATYPE(PlayListEntity)

