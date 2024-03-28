//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>
#include <map>

#include <QString>
#include <QSqlQuery>

#include <base/object_pool.h>

#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <widget/util/str_utilts.h>
#include <widget/playlistentity.h>

class SqlException final : public Exception {
public:
	explicit SqlException(QSqlError error);

	const char* what() const noexcept override;
};

struct XAMP_WIDGET_SHARED_EXPORT AlbumStats {
	int32_t songs{0};
	double durations{0};
	int32_t year{0};
	size_t file_size{0};
};

struct XAMP_WIDGET_SHARED_EXPORT ArtistStats {
	int32_t albums{0};
	int32_t tracks{0};
	double durations{0};
};

inline constexpr auto kMaxDatabasePoolSize = 8;

inline constexpr int32_t kInvalidDatabaseId = -1;

inline constexpr auto kDefaultPlaylistId = 1;
inline constexpr auto kAlbumPlaylistId = 2;
inline constexpr auto kCdPlaylistId = 3;
inline constexpr auto kYtMusicSearchPlaylistId = 4;
inline constexpr auto kMaxExistPlaylist = 5;

enum PlayingState {
	PLAY_CLEAR = 0,
	PLAY_PLAYING,
	PLAY_PAUSE,
};

enum class StoreType {
	LOCAL_STORE = -1,
	CLOUD_STORE = -2,
	CLOUD_SEARCH_STORE = -3,
};

class SqlQuery : public QSqlQuery {
public:
	explicit SqlQuery(const QString& query = QString(), const QSqlDatabase& db = QSqlDatabase())
		: QSqlQuery(query, db) {
	}

	explicit SqlQuery(const QSqlDatabase& db)
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

	Q_DISABLE_COPY(Database)

	QSqlDatabase& database();

	void close();

	void open();

	bool transaction();

	bool commit();

	void rollback();

	bool dropAllTable();

	QStringList getGenres() const;

	QStringList getCategories() const;

	QStringList getYears() const;

	std::map<int32_t, int32_t> getPlaylistIndex(StoreType type);

	void setAlbumCover(int32_t album_id, const QString& cover_id);

	std::optional<AlbumStats> getAlbumStats(int32_t album_id) const;

	std::optional<ArtistStats> getArtistStats(int32_t artist_id) const;

	int32_t addOrUpdateMusic(const TrackInfo& track_info);

	void updateMusic(int32_t music_id, const TrackInfo& track_info);

	int32_t addOrUpdateArtist(const QString& artist);

	void updateArtistEnglishName(const QString& artist, const QString& en_name);

	void updateArtistCoverId(int32_t artist_id, const QString& cover_id);

	void updateMusicFilePath(int32_t music_id, const QString& file_path);

	void addOrUpdateLyrics(int32_t music_id, const QString& lyrc, const QString& trlyrc);

	std::optional<std::tuple<QString, QString>> getLyrics(int32_t music_id);

	void updateMusicRating(int32_t music_id, int32_t rating);

	void updateAlbumHeart(int32_t album_id, uint32_t heart);

	void updateMusicHeart(int32_t music_id, uint32_t heart);

	void updateMusicTitle(int32_t music_id, const QString& title);

	void updateMusicPlays(int32_t music_id);

	void addOrUpdateTrackLoudness(int32_t album_id,
	                              int32_t artist_id,
	                              int32_t music_id,
	                              double track_loudness);

	void updateReplayGain(int32_t music_id,
	                      double album_rg_gain,
	                      double album_peak,
	                      double track_rg_gain,
	                      double track_peak);

	int32_t addOrUpdateAlbum(const QString& album, int32_t artist_id, int64_t album_time, uint32_t year,
							 StoreType store_type,
	                         const QString& disc_id = kEmptyString,
	                         const QString& album_genre = kEmptyString,
	                         bool is_hires = false);

	void addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const;

	void addOrUpdateAlbumCategory(int32_t album_id, const QString& category) const;

	void addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const;

	void addOrUpdateMusicLoudness(int32_t album_id, int32_t artist_id, int32_t music_id,
	                              double track_loudness = 0) const;

	int32_t getAlbumIdByDiscId(const QString& disc_id) const;

	void updateAlbumByDiscId(const QString& disc_id, const QString& album);

	void updateArtistByDiscId(const QString& disc_id, const QString& artist);

	void updateAlbum(int32_t album_id, const QString& album);

	void updateArtist(int32_t artist_id, const QString& artist);

	QString getAlbumCoverId(int32_t album_id) const;

	QString getMusicCoverId(int32_t music_id) const;

	int32_t getAlbumId(const QString& album) const;

	QString getAlbumCoverId(const QString& album) const;

	void setMusicCover(int32_t music_id, const QString& cover_id);

	QString getArtistCoverId(int32_t artist_id) const;

	void setPlaylistName(int32_t playlist_id, const QString& name);

	void removeAlbum(int32_t album_id);

	void removeAlbumMusic(int32_t album_id, int32_t music_id);

	void forEachAlbumCover(std::function<void(QString)>&& fun, int limit) const;

	void forEachPlaylist(std::function<void(int32_t, int32_t, StoreType, QString, QString)>&& fun);

	void forEachAlbumMusic(int32_t album_id, std::function<void(const PlayListEntity&)>&& fun);

	void forEachAlbum(std::function<void(int32_t)>&& fun);

	std::optional<QString> getAlbumFirstMusicFilePath(int32_t album_id);

	void removeAllArtist();

	void removeArtistId(int32_t artist_id);

	void removeMusic(int32_t music_id);

	void removeTrackLoudnessMusicId(int32_t music_id);

	void removeMusic(const QString& file_path);

	void removePlaylist(int32_t playlist_id);

	void removePlaylistAllMusic(int32_t playlist_id);

	void updatePlaylistMusicChecked(int32_t playlist_music_id, bool is_checked);

	void removePlaylistMusic(int32_t playlist_id, const QVector<int>& select_music_ids);

	bool isPlaylistExist(int32_t playlist_id) const;

	int32_t addPlaylist(const QString& name, int32_t index, StoreType store_type, const QString& cloud_playlist_id= kEmptyString);

	void setPlaylistIndex(int32_t playlist_id, int32_t play_index, StoreType store_type);

	void addMusicToPlaylist(int32_t music_id, int32_t playlist_id, int32_t album_id) const;

	void addMusicToPlaylist(const QList<int32_t>& music_id, int32_t playlist_id) const;

	void setNowPlaying(int32_t playlist_id, int32_t music_id);

	void clearNowPlaying(int32_t playlist_id);

	void clearNowPlaying(int32_t playlist_id, int32_t music_id);

	void clearNowPlayingSkipMusicId(int32_t playlist_id, int32_t skip_playlist_music_id);

	void setNowPlayingState(int32_t playlist_id, int32_t playlist_music_id, PlayingState playing);

private:
	static PlayListEntity fromSqlQuery(const SqlQuery& query);

	void removeAlbumArtist(int32_t album_id);

	void removeAlbumMusicAlbum(int32_t album_id);

	void removeAlbumCategory(int32_t album_id);

	void removeAlbumMusicId(int32_t music_id);

	void removePlaylistMusics(int32_t music_id);

	void removeAlbumArtistId(int32_t artist_id);

	void createTableIfNotExist();

	QString getVersion() const;

	QString connection_name_;
	QSqlDatabase db_;
	LoggerPtr logger_;
};

class XAMP_WIDGET_SHARED_EXPORT DatabaseFactory {
public:
	Database* Create() {
		auto* database = new Database(getDatabaseId());
		database->open();
		return database;
	}

private:
	static QString getDatabaseId();
};

using PooledDatabasePtr = std::shared_ptr<ObjectPool<Database, DatabaseFactory>>;

XAMP_WIDGET_SHARED_EXPORT PooledDatabasePtr getPooledDatabase(int32_t pool_size = kMaxDatabasePoolSize);

#define qMainDb SharedSingleton<Database>::GetInstance()
