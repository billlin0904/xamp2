//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>

#include <QString>
#include <QSqlDatabase>

#include <widget/widget_shared.h>
#include <widget/playlistentity.h>

class SqlException final : public Exception {
public:
    explicit SqlException(QSqlError error);

    virtual const char* what() const noexcept override;
};

#define IgnoreSqlError(expr) \
    try {\
    expr;\
    }\
    catch (const Exception& e) {\
    XAMP_LOG_DEBUG(e.what());\
    }

struct AlbumStats {
    int32_t tracks{ 0 };
    double durations{ 0 };
    int32_t year{ 0 };
};

struct ArtistStats {
    int32_t albums{ 0 };
    int32_t tracks{ 0 };
    double durations{ 0 };
};

inline constexpr int32_t kInvalidId = -1;

inline constexpr auto kDefaultPlaylistId = 1;
inline constexpr auto kDefaultPodcastPlaylistId = 2;
inline constexpr auto kDefaultFileExplorerPlaylistId = 3;
inline constexpr auto kDefaultAlbumPlaylistId = 4;
inline constexpr auto kDefaultCdPlaylistId = 5;

class Database final {
public:
    Database();

    ~Database();

    void open(const QString& file_name);

    void flush();

    int32_t addTable(const QString& name, int32_t table_index, int32_t playlist_id);

    int32_t addPlaylist(const QString& name, int32_t playlistIndex);

    void setAlbumCover(int32_t album_id, const QString& album, const QString& cover_id);

    std::optional<AlbumStats> getAlbumStats(int32_t album_id) const;

    std::optional<ArtistStats> getArtistStats(int32_t artist_id) const;

    void addTablePlaylist(int32_t tableId, int32_t playlist_id);

    int32_t addOrUpdateMusic(const Metadata& medata, int32_t playlist_id);

    int32_t addOrUpdateArtist(const QString& artist);

    void updateArtistCoverId(int32_t artist_id, const QString& coverId);

    void updateMusicFilePath(int32_t music_id, const QString& file_path);

    void updateMusicRating(int32_t music_id, int32_t rating);

    void updateReplayGain(int music_id,
                    double album_rg_gain,
                    double album_peak,
                    double track_rg_gain,
                    double track_peak);

    int32_t addOrUpdateAlbum(const QString& album, int32_t artist_id, int64_t album_time, bool is_podcast);

    void addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const;

    void addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id);

    QString getAlbumCoverId(int32_t album_id) const;

    QString getAlbumCoverId(const QString& album) const;

    QString getArtistCoverId(int32_t artist_id) const;

    void setTableName(int32_t table_id, const QString &name);

    void removeAlbum(int32_t album_id);

    void forEachTable(std::function<void(int32_t, int32_t, int32_t, QString)>&& fun);

    void forEachPlaylist(std::function<void(int32_t, int32_t, QString)>&& fun);

    void forEachAlbumArtistMusic(int32_t album_id, int32_t artist_id, std::function<void(PlayListEntity const&)>&& fun);

    void forEachAlbumMusic(int32_t album_id, std::function<void(PlayListEntity const &)> &&fun);

    void forEachAlbum(std::function<void(int32_t)>&& fun);

    void removeAllArtist();

    void removeArtistId(int32_t artist_id);

    void removeMusic(int32_t music_id);

    void removeMusic(QString const& file_path);

    void removePlaylistAllMusic(int32_t playlist_id);

    void removeMusic(int32_t playlist_id, const QVector<int32_t>& select_music_ids);
	
    void removePlaylistMusic(int32_t playlist_id, const QVector<int>& select_music_ids);

    int32_t findTablePlaylistId(int32_t table_id) const;

    bool isPlaylistExist(int32_t playlist_id) const;

    void addMusicToPlaylist(int32_t music_id, int32_t playlist_id) const;

    void addMusicToPlaylist(const Vector<int32_t> & music_id, int32_t playlist_id) const;

    void setNowPlaying(int32_t playlist_id, int32_t music_id);

    void clearNowPlaying(int32_t playlist_id);

    void clearNowPlaying(int32_t playlist_id, int32_t music_id);
private:
    void removeAlbumArtist(int32_t album_id);

    void removeAlbumMusicId(int32_t music_id);

    void removePlaylistMusics(int32_t music_id);

    void removeAlbumArtistId(int32_t artist_id);

    void addAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const;

    void createTableIfNotExist();

    QString dbname_;
    QSqlDatabase db_;
};

#define qDatabase SharedSingleton<Database>::GetInstance()