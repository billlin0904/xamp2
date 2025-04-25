//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <QString>
#include <QModelIndex>
#include <QVariant>
#include <QFileInfo>
#include <QUrl>

#include <widget/util/str_util.h>
#include <base/assert.h>
#include <widget/widget_shared_global.h>

struct XAMP_WIDGET_SHARED_EXPORT PlayListEntity final {
    bool is_cue_file{ false };
    bool heart{ false };
    int32_t playing{0};
    int32_t music_id{0};
    int32_t playlist_music_id{0};
    int32_t album_id{0};
    int32_t artist_id{0};    
    uint32_t track{0};
    uint32_t bit_rate{0};
    uint32_t sample_rate{0};
    uint32_t rating{0};    
    uint32_t year{0};
    uint64_t file_size{0};
    double duration{0};
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
    QString yt_music_album_id;

    std::optional<QString> music_cover_id;    
    std::optional<double> offset;
    std::optional<double> album_replay_gain;
    std::optional<double> album_peak;
    std::optional<double> track_replay_gain;
    std::optional<double> track_peak;
    std::optional<double> track_loudness;

    XAMP_NO_DISCARD bool isHttpUrl() const {
        const auto scheme = QUrl(file_path).scheme();
        return scheme == "https"_str || scheme == "http"_str;
    }

    XAMP_NO_DISCARD bool isFilePath() const {
        return !QFileInfo(file_path).suffix().isEmpty();
    }

    XAMP_NO_DISCARD QString validCoverId() const {
        auto id = music_cover_id ? music_cover_id.value() : kEmptyString;
        if (id.isEmpty() || id.isNull()) {
            id = cover_id;
        }
        return id;
    }

    XAMP_NO_DISCARD PlayListEntity cleanup() const {
        PlayListEntity temp;
        auto feat_pos = temp.title.indexOf("feat"_str);
        if (feat_pos != -1) {
            temp.title = temp.title.left(feat_pos).trimmed();
        }
        if (temp.title.isEmpty()) {
            temp.title = title;
        }
        return temp;
    }

    XAMP_NO_DISCARD uint32_t getDopSampleRate() const {
        XAMP_ASSERT(this->sample_rate > 0);
        uint32_t dop_sample_rate = 0;
        if (this->sample_rate >= 2822400) {
            dop_sample_rate = 88200;
        }
        else {
            dop_sample_rate = sample_rate;
        }
        return dop_sample_rate;
    }
};

// for QVariant轉換使用
Q_DECLARE_METATYPE(PlayListEntity)

QVariant indexValue(const QModelIndex& index, const QModelIndex& src, int i);

QVariant indexValue(const QModelIndex& index, int i);

PlayListEntity getEntity(const QModelIndex& index);
