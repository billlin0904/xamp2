//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>

#include <QString>
#include <QSqlDatabase>

#include <widget/xmessagebox.h>
#include <widget/widget_shared.h>
#include <widget/str_utilts.h>
#include <widget/playlistentity.h>

class SqlException final : public Exception {
public:
    explicit SqlException(QSqlError error);

    virtual const char* what() const noexcept override;
};

#define IGNORE_DB_EXCEPTION(expr) \
    try {\
		(expr);\
    }\
    catch (const Exception& e) {\
		XAMP_LOG_DEBUG(e.what());\
    }

#define CATCH_DB_EXCEPTION(expr) \
    try {\
		(expr);\
    }\
    catch (SqlException const& e) {\
		XMessageBox::showBug(e);\
    }

struct AlbumStats {
    int32_t tracks{ 0 };
    double durations{ 0 };
    int32_t year{ 0 };
    size_t file_size{ 0 };
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

enum PlayingState {
	PLAY_CLEAR = 0,
    PLAY_PLAYING,
    PLAY_PAUSE,
};

class Database final {
public:
    static Database& getThreadDatabase();

    Database();

    ~Database();

    QSqlDatabase& database();

    void open();

    void transaction();

    void commit();

    void rollback();

    int32_t addTable(const QString& name, int32_t table_index, int32_t playlist_id);

    int32_t addPlaylist(const QString& name, int32_t playlist_index);

    void setAlbumCover(int32_t album_id, const QString& cover_id);

    void setAlbumCover(int32_t album_id, const QString& album, const QString& cover_id);

    std::optional<AlbumStats> getAlbumStats(int32_t album_id) const;

    std::optional<ArtistStats> getArtistStats(int32_t artist_id) const;

    void addTablePlaylist(int32_t table_id, int32_t playlist_id);

    int32_t addOrUpdateMusic(const TrackInfo& track_info);

    int32_t addOrUpdateArtist(const QString& artist);

    void updateArtistCoverId(int32_t artist_id, const QString& cover_id);

    void updateMusicFilePath(int32_t music_id, const QString& file_path);

    void updateMusicRating(int32_t music_id, int32_t rating);

    void updateMusicTitle(int32_t music_id, const QString& title);

    void addOrUpdateTrackLoudness(int32_t album_id,
        int32_t artist_id,
        int32_t music_id, 
        double track_loudness);

    void updateReplayGain(int32_t music_id,
        double album_rg_gain,
        double album_peak,
        double track_rg_gain,
        double track_peak);

    ForwardList<PlayListEntity> getPlayListEntityFromPathHash(size_t path_hash) const;

    size_t getParentPathHash(const QString& parent_path) const;

    int32_t addOrUpdateAlbum(const QString& album, int32_t artist_id, int64_t album_time, bool is_podcast, const QString& disc_id = qEmptyString);

    void addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const;

    void addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const;

    void addOrUpdateMusicLoudness(int32_t album_id, int32_t artist_id, int32_t music_id, double track_loudness = 0) const;

    int32_t getAlbumIdByDiscId(const QString& disc_id) const;

    void updateAlbumByDiscId(const QString& disc_id, const QString& album);

    void updateArtistByDiscId(const QString& disc_id, const QString& artist);

    QString getAlbumCoverId(int32_t album_id) const;

    QString getAlbumCoverId(const QString& album) const;

    QString getArtistCoverId(int32_t artist_id) const;

    void setTableName(int32_t table_id, const QString &name);

    void removeAlbum(int32_t album_id);

    void forEachTable(std::function<void(int32_t, int32_t, int32_t, QString)>&& fun);

    void forEachPlaylist(std::function<void(int32_t, int32_t, QString)>&& fun);

    void forEachAlbumMusic(int32_t album_id, std::function<void(PlayListEntity const &)> &&fun);

    void forEachAlbum(std::function<void(int32_t)>&& fun);

    void removeAllArtist();

    void removeArtistId(int32_t artist_id);

    void removeMusic(int32_t music_id);

    void removeTrackLoudnessMusicId(int32_t music_id);

    void removeMusic(QString const& file_path);

    void removePlaylistAllMusic(int32_t playlist_id);

    void removeMusic(int32_t playlist_id, const QVector<int32_t>& select_music_ids);
	
    void removePlaylistMusic(int32_t playlist_id, const QVector<int>& select_music_ids);

    int32_t findTablePlaylistId(int32_t table_id) const;

    bool isPlaylistExist(int32_t playlist_id) const;

    void addMusicToPlaylist(int32_t music_id, int32_t playlist_id, int32_t album_id) const;

    void addMusicToPlaylist(const ForwardList<int32_t> & music_id, int32_t playlist_id) const;

    void setNowPlaying(int32_t playlist_id, int32_t music_id);

    void clearNowPlaying(int32_t playlist_id);

    void clearNowPlaying(int32_t playlist_id, int32_t music_id);

    void clearNowPlayingSkipMusicId(int32_t playlist_id, int32_t skip_playlist_music_id);

    void setNowPlayingState(int32_t playlist_id, int32_t playlist_music_id, PlayingState playing);
private:
    static PlayListEntity queryToPlayListEntity(const QSqlQuery& query);

    void removeAlbumArtist(int32_t album_id);

    void removeAlbumMusicId(int32_t music_id);

    void removePlaylistMusics(int32_t music_id);

    void removeAlbumArtistId(int32_t artist_id);

    void createTableIfNotExist();

    QString getVersion() const;

    QString connection_name_;
    QSqlDatabase db_;
    LoggerPtr logger_;
};

//#define qDatabase SharedSingleton<Database>::GetInstance()
#define qDatabase Database::getThreadDatabase()