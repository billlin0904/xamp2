//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <QString>
#include <QModelIndex>
#include <QVariant>
#include <QFileInfo>
#include <QUrl>

#include <widget/util/str_utilts.h>
#include <widget/widget_shared_global.h>

struct XAMP_WIDGET_SHARED_EXPORT PlayListEntity final {
    int32_t playing{0};
    int32_t music_id{0};
    int32_t playlist_music_id{0};
    int32_t album_id{0};
    int32_t artist_id{0};
    uint32_t bit{0};
    uint32_t track{0};
    uint32_t bit_rate{0};
    uint32_t sample_rate{0};
    uint32_t rating{0};
    uint32_t heart{0};
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

    std::optional<QString> music_cover_id;
    
    std::optional<double> offset;

    std::optional<double> album_replay_gain;
    std::optional<double> album_peak;
    std::optional<double> track_replay_gain;
    std::optional<double> track_peak;
    std::optional<double> track_loudness;

    [[nodiscard]] bool isHttpUrl() const {
        const auto scheme = QUrl(file_path).scheme();
        return scheme == qTEXT("https") || scheme == qTEXT("http");
    }

    [[nodiscard]] bool isFilePath() const {
        return !QFileInfo(file_path).suffix().isEmpty();
    }

    [[nodiscard]] QString validCoverId() const {
        auto id = music_cover_id ? music_cover_id.value() : kEmptyString;
        if (id.isEmpty() || id.isNull()) {
            id = cover_id;
        }
        return id;
    }
};

// for QVariant轉換使用
Q_DECLARE_METATYPE(PlayListEntity)

QVariant indexValue(const QModelIndex& index, const QModelIndex& src, int i);

QVariant indexValue(const QModelIndex& index, int i);

PlayListEntity getEntity(const QModelIndex& index);
