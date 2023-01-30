//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
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
		XMessageBox::ShowBug(e);\
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
    static Database& GetThreadDatabase();

    Database();

    ~Database();

    QSqlDatabase& database();

    void open();

    void transaction();

    void commit();

    void rollback();

    int32_t AddTable(const QString& name, int32_t table_index, int32_t playlist_id);

    int32_t AddPlaylist(const QString& name, int32_t playlist_index);

    void SetAlbumCover(int32_t album_id, const QString& cover_id);

    void SetAlbumCover(int32_t album_id, const QString& album, const QString& cover_id);

    std::optional<AlbumStats> GetAlbumStats(int32_t album_id) const;

    std::optional<ArtistStats> GetArtistStats(int32_t artist_id) const;

    void AddTablePlaylist(int32_t table_id, int32_t playlist_id);

    int32_t AddOrUpdateMusic(const TrackInfo& track_info);

    int32_t AddOrUpdateArtist(const QString& artist);

    void UpdateArtistCoverId(int32_t artist_id, const QString& cover_id);

    void UpdateMusicFilePath(int32_t music_id, const QString& file_path);

    void UpdateMusicRating(int32_t music_id, int32_t rating);

    void UpdateMusicTitle(int32_t music_id, const QString& title);

    void AddOrUpdateTrackLoudness(int32_t album_id,
        int32_t artist_id,
        int32_t music_id, 
        double track_loudness);

    void UpdateReplayGain(int32_t music_id,
        double album_rg_gain,
        double album_peak,
        double track_rg_gain,
        double track_peak);

    ForwardList<PlayListEntity> GetPlayListEntityFromPathHash(size_t path_hash) const;

    size_t GetParentPathHash(const QString& parent_path) const;

    int32_t AddOrUpdateAlbum(const QString& album, int32_t artist_id, int64_t album_time, bool is_podcast, const QString& disc_id = qEmptyString);

    void AddOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const;

    void AddOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const;

    void AddOrUpdateMusicLoudness(int32_t album_id, int32_t artist_id, int32_t music_id, double track_loudness = 0) const;

    int32_t GetAlbumIdByDiscId(const QString& disc_id) const;

    void UpdateAlbumByDiscId(const QString& disc_id, const QString& album);

    void UpdateArtistByDiscId(const QString& disc_id, const QString& artist);

    QString GetAlbumCoverId(int32_t album_id) const;

    QString GetAlbumCoverId(const QString& album) const;

    QString GetArtistCoverId(int32_t artist_id) const;

    void SetTableName(int32_t table_id, const QString &name);

    void RemoveAlbum(int32_t album_id);

    void ForEachTable(std::function<void(int32_t, int32_t, int32_t, QString)>&& fun);

    void ForEachPlaylist(std::function<void(int32_t, int32_t, QString)>&& fun);

    void ForEachAlbumMusic(int32_t album_id, std::function<void(PlayListEntity const &)> &&fun);

    void ForEachAlbum(std::function<void(int32_t)>&& fun);

    std::optional<QString> GetAlbumFirstMusicFilePath(int32_t album_id);

    void RemoveAllArtist();

    void RemoveArtistId(int32_t artist_id);

    void RemoveMusic(int32_t music_id);

    void RemoveTrackLoudnessMusicId(int32_t music_id);

    void RemoveMusic(QString const& file_path);

    void RemovePlaylistAllMusic(int32_t playlist_id);

    void RemoveMusic(int32_t playlist_id, const QVector<int32_t>& select_music_ids);
	
    void RemovePlaylistMusic(int32_t playlist_id, const QVector<int>& select_music_ids);

    int32_t FindTablePlaylistId(int32_t table_id) const;

    bool IsPlaylistExist(int32_t playlist_id) const;

    void AddMusicToPlaylist(int32_t music_id, int32_t playlist_id, int32_t album_id) const;

    void AddMusicToPlaylist(const ForwardList<int32_t> & music_id, int32_t playlist_id) const;

    void SetNowPlaying(int32_t playlist_id, int32_t music_id);

    void ClearNowPlaying(int32_t playlist_id);

    void ClearNowPlaying(int32_t playlist_id, int32_t music_id);

    void ClearNowPlayingSkipMusicId(int32_t playlist_id, int32_t skip_playlist_music_id);

    void SetNowPlayingState(int32_t playlist_id, int32_t playlist_music_id, PlayingState playing);
private:
    static PlayListEntity QueryToPlayListEntity(const QSqlQuery& query);

    void RemoveAlbumArtist(int32_t album_id);

    void RemoveAlbumMusicId(int32_t music_id);

    void RemovePlaylistMusics(int32_t music_id);

    void RemoveAlbumArtistId(int32_t artist_id);

    void CreateTableIfNotExist();

    QString GetVersion() const;

    QString connection_name_;
    QSqlDatabase db_;
    LoggerPtr logger_;
};

#define qDatabase Database::GetThreadDatabase()