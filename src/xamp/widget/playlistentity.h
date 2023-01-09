//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <QString>

struct PlayListEntity final {
    int32_t playing{0};
    int32_t music_id{0};
    int32_t playlist_music_id{0};
    int32_t album_id{0};
    int32_t artist_id{0};
    uint32_t track{0};
    uint32_t bitrate{0};
    uint32_t samplerate{0};
    uint32_t rating{0};
    uint32_t year{0};
    uint64_t file_size{ 0 };
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
    QString file_ext;
    QString cover_id;
    QString file_name;
    QString genre;
    QString comment;
};

// for QVariantÂà´«¨Ï¥Î
Q_DECLARE_METATYPE(PlayListEntity)

