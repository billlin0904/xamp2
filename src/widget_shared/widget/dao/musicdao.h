//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <widget/widget_shared.h>
#include <widget/database.h>

namespace dao {

class XAMP_WIDGET_SHARED_EXPORT MusicDao final {
public:
    MusicDao();

    explicit MusicDao(QSqlDatabase& db);

    int32_t addOrUpdateMusic(const TrackInfo& track_info);
    std::optional<int32_t> getMusicId(const QString& file_path) const;
    void updateMusic(int32_t music_id, const TrackInfo& track_info);
    void updateMusicFilePath(int32_t music_id, const QString& file_path);
    void updateMusicReplayGain(int32_t music_id, double album_gain, double album_peak, double track_gain, double track_peak);
    void addOrUpdateLyrics(int32_t music_id, const QString& lyrc, const QString& trlyrc);
    std::optional<std::tuple<QString, QString>> getLyrics(int32_t music_id);
    void updateMusicRating(int32_t music_id, int32_t rating);
    void updateMusicHeart(int32_t music_id, uint32_t heart);
    void updateMusicTitle(int32_t music_id, const QString& title);
    void updateMusicPlays(int32_t music_id);
    QString getMusicCoverId(int32_t music_id) const;
    QString getMusicFilePath(int32_t music_id) const;
    void setMusicCover(int32_t music_id, const QString& cover_id);
    void removeMusic(int32_t music_id) const;
    void removeTrackLoudnessMusicId(int32_t music_id);
    void removeMusic(const QString& file_path);
    void addOrUpdateMusicLoudness(int32_t album_id, int32_t artist_id, int32_t music_id, double track_loudness = 0) const;
    void removeCoverId();
private:
    QSqlDatabase& db_;
};

}