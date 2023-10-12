//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>

#include <QString>
#include <QSqlQuery>

#include <base/object_pool.h>

#include <widget/widget_shared_global.h>
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
    }

struct XAMP_WIDGET_SHARED_EXPORT AlbumStats {
    int32_t songs{ 0 };
    double durations{ 0 };
    int32_t year{ 0 };
    size_t file_size{ 0 };
};

struct XAMP_WIDGET_SHARED_EXPORT ArtistStats {
    int32_t albums{ 0 };
    int32_t tracks{ 0 };
    double durations{ 0 };
};

inline constexpr auto kMaxDatabasePoolSize = 8;

inline constexpr int32_t kInvalidDatabaseId = -1;

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

class SqlQuery : public QSqlQuery {
public:
    explicit SqlQuery(const QString& query = QString(), QSqlDatabase db = QSqlDatabase())
        : QSqlQuery(query, db) {
    }

    explicit SqlQuery(QSqlDatabase db)
	    : QSqlQuery(db) {
    }

    ~SqlQuery() {
        QSqlQuery::finish();
        QSqlQuery::clear();
    }
};

class XAMP_WIDGET_SHARED_EXPORT Database final {
public:
    explicit Database(const QString& name);

    Database();

    ~Database();

    QSqlDatabase& database();

    void close();

    void open();

    bool transaction();

    bool commit();

    void rollback();

    bool DropAllTable();

    QStringList GetGenres() const;

    QStringList GetCategories() const;

    QStringList GetYears() const;

    int32_t AddTable(const QString& name, int32_t table_index, int32_t playlist_id);

    int32_t AddPlaylist(const QString& name, int32_t playlist_index);

    void SetAlbumCover(int32_t album_id, const QString& cover_id);

    void SetAlbumCover(int32_t album_id, const QString& album, const QString& cover_id);

    std::optional<AlbumStats> GetAlbumStats(int32_t album_id) const;

    std::optional<ArtistStats> GetArtistStats(int32_t artist_id) const;

    void AddTablePlaylist(int32_t table_id, int32_t playlist_id);

    int32_t AddOrUpdateMusic(const TrackInfo& track_info);

    int32_t AddOrUpdateArtist(const QString& artist);

    void UpdateArtistEnglishName(const QString& artist, const QString& en_name);

    void UpdateArtistCoverId(int32_t artist_id, const QString& cover_id);

    void UpdateMusicFilePath(int32_t music_id, const QString& file_path);

    void AddOrUpdateLyrc(int32_t music_id, const QString& lyrc, const QString& trlyrc);

    std::optional<std::tuple<QString, QString>> GetLyrc(int32_t music_id);

    void UpdateMusicRating(int32_t music_id, int32_t rating);

    void UpdateAlbumHeart(int32_t album_id, uint32_t heart);

    void UpdateMusicHeart(int32_t music_id, uint32_t heart);

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

    void ClearPendingPlaylist();

    void ClearPendingPlaylist(int32_t playlist_id);

    void AddPendingPlaylist(int32_t playlist_musics_id, int32_t playlist_id) const;

    std::pair<int32_t, int32_t> GetFirstPendingPlaylistMusic(int32_t playlist_id);

    void DeletePendingPlaylistMusic(int32_t pending_playlist_id);

    int32_t AddOrUpdateAlbum(const QString& album, int32_t artist_id, int64_t album_time, uint32_t year, bool is_podcast,
        const QString& disc_id = kEmptyString,
        const QString& album_genre = kEmptyString);

    void AddOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const;

    void AddOrUpdateAlbumCategory(int32_t album_id, const QString & category) const;

    void AddOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const;

    void AddOrUpdateMusicLoudness(int32_t album_id, int32_t artist_id, int32_t music_id, double track_loudness = 0) const;

    int32_t GetAlbumIdByDiscId(const QString& disc_id) const;

    void UpdateAlbumByDiscId(const QString& disc_id, const QString& album);

    void UpdateArtistByDiscId(const QString& disc_id, const QString& artist);

    QString GetAlbumCoverId(int32_t album_id) const;

    int32_t GetAlbumId(const QString& album) const;

    QString GetAlbumCoverId(const QString& album) const;

    QString GetArtistCoverId(int32_t artist_id) const;

    void SetTableName(int32_t table_id, const QString &name);

    void RemoveAlbum(int32_t album_id);

    void ForEachAlbumCover(std::function<void(QString)>&& fun, int limit) const;

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

    void RemovePendingListMusic(int32_t playlist_id);
	
    void RemovePlaylistMusic(int32_t playlist_id, const QVector<int>& select_music_ids);

    int32_t FindTablePlaylistId(int32_t table_id) const;

    bool IsPlaylistExist(int32_t playlist_id) const;

    void AddMusicToPlaylist(int32_t music_id, int32_t playlist_id, int32_t album_id) const;

    void AddMusicToPlaylist(const QList<int32_t> & music_id, int32_t playlist_id) const;

    void SetNowPlaying(int32_t playlist_id, int32_t music_id);

    void ClearNowPlaying(int32_t playlist_id);

    void ClearNowPlaying(int32_t playlist_id, int32_t music_id);

    void ClearNowPlayingSkipMusicId(int32_t playlist_id, int32_t skip_playlist_music_id);

    void SetNowPlayingState(int32_t playlist_id, int32_t playlist_music_id, PlayingState playing);
private:
    static PlayListEntity FromSqlQuery(const SqlQuery& query);

    void RemoveAlbumArtist(int32_t album_id);

    void RemoveAlbumMusicAlbum(int32_t album_id);

    void RemoveAlbumCategory(int32_t album_id);

    void RemoveAlbumMusicId(int32_t music_id);

    void RemovePlaylistMusics(int32_t music_id);

    void RemoveAlbumArtistId(int32_t artist_id);

    void CreateTableIfNotExist();

    QString GetVersion() const;

    QString connection_name_;
    QSqlDatabase db_;
    LoggerPtr logger_;
};

class DatabaseFactory {
public:
    Database* Create() {
        auto* database = new Database(GetDatabaseId());
        database->open();
        return database;
    }
private:
    static QString GetDatabaseId();
};

using PooledDatabasePtr = std::shared_ptr<ObjectPool<Database, DatabaseFactory>>;

PooledDatabasePtr GetPooledDatabase(int32_t pool_size = kMaxDatabasePoolSize);

#define qMainDb SharedSingleton<Database>::GetInstance()