//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <QString>

#include <widget/widget_shared_global.h>

struct XAMP_WIDGET_SHARED_EXPORT PlayListEntity final {
    int32_t playing{0};
    int32_t music_id{0};
    int32_t playlist_music_id{0};
    int32_t album_id{0};
    int32_t artist_id{0};
    uint32_t track{0};
    uint32_t bit_rate{0};
    uint32_t sample_rate{0};
    uint32_t rating{0};
    uint32_t heart{0};
    uint64_t file_size{0};
    double duration{0};
    double album_replay_gain{0};
    double album_peak{0};
    double track_replay_gain{0};
    double track_peak{0};
    double track_loudness{0};
    uint64_t timestamp{0};
    QString disc_id;
    QString file_path;
    QString parent_path;
    QString title;
    QString album;
    QString artist;
    QString file_extension;
    QString cover_id;
    QString file_name;
    QString genre;
    QString comment;
    QString lyrc;
    QString trlyrc;
};

// for QVariantÂà´«¨Ï¥Î
Q_DECLARE_METATYPE(PlayListEntity)

