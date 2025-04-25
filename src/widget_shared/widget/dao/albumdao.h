//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <widget/widget_shared.h>
#include <widget/database.h>
#include <widget/playlistentity.h>

namespace dao {

class XAMP_WIDGET_SHARED_EXPORT AlbumDao final {
public:
    AlbumDao();

    explicit AlbumDao(QSqlDatabase& db);

    void setAlbumCover(int32_t album_id, const QString& cover_id);
    std::optional<AlbumStats> getAlbumStats(int32_t album_id) const;
    int32_t addOrUpdateAlbum(const QString& album,
        int32_t artist_id, 
        int64_t album_time,
        uint32_t year,
        StoreType store_type,
        const QString& disc_id = kEmptyString,
        bool is_hires = false);
    void updateAlbum(int32_t album_id, const QString& album);
    void updateAlbumHeart(int32_t album_id, uint32_t heart);
    void updateAlbumPlays(int32_t album_id);
    QString getAlbumCoverId(int32_t album_id) const;
    int32_t getAlbumId(const QString& album) const;
    QString getAlbumCoverId(const QString& album) const;
    void removeAlbum(int32_t album_id);
    void addAlbumCategory(int32_t album_id, const QString& category) const;
    void addOrUpdateAlbumCategory(int32_t album_id, const QString& category) const;
    void addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const;
    int32_t getAlbumIdByDiscId(const QString& disc_id) const;
    void updateAlbumByDiscId(const QString& disc_id, const QString& album);
    void updateAlbumByDiscId(const QString& disc_id, int32_t album_id);
    
    std::optional<QString> getAlbumFirstMusicFilePath(int32_t album_id) const;

    void removeAlbumMusic(int32_t album_id, int32_t music_id);

    int32_t getAlbumIdFromAlbumMusic(int32_t music_id);
    void addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id);
    void addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id);
    void updateReplayGain(int32_t music_id,
        double album_rg_gain,
        double album_peak,
        double track_rg_gain,
        double track_peak);
    void addOrUpdateTrackLoudness(int32_t album_id, int32_t artist_id, int32_t music_id, double track_loudness);
    QStringList getCategories() const;
    QStringList getYears() const;
    void forEachAlbumMusic(int32_t album_id, std::function<void(PlayListEntity const&)>&& fun);
    void updateAlbumSelectState(int32_t album_id, bool state);
    QList<int32_t> getSelectedAlbums();
    void forEachAlbum(std::function<void(int32_t)>&& fun);
    void forEachAlbumCover(std::function<void(const QString&)>&& fun, int32_t limit = 10);
    void removeAlbumCategory(int32_t album_id);
    void removeAlbumMusicAlbum(int32_t album_id);
    void removeAlbumArtist(int32_t album_id);
    int32_t getRandomMusicId(int32_t album_id, PRNG& rng);
    int32_t getRandomAlbumId(int32_t album_id, PRNG& rng);

    QList<QString> getAlbumTags();
private:
    QSqlDatabase& db_;
};

}