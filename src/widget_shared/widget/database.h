//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>
#include <map>

#include <QString>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

#include <base/object_pool.h>

#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <widget/util/str_util.h>
#include <widget/playlistentity.h>

class SqlQuery final : public QSqlQuery {
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

class SqlException final : public Exception {
public:
	explicit SqlException(const SqlQuery &query);

	explicit SqlException(QSqlError error);

	const char* what() const noexcept override;
};

#define DbIfFailedThrow(query, sql) \
    do {\
    if (!query.exec(sql)) {\
    throw SqlException(query);\
    }\
    } while (false)

#define DbIfFailedThrow1(query) \
    do {\
    if (!query.exec()) {\
    throw SqlException(query);\
    }\
    } while (false)

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

constexpr auto kMaxDatabasePoolSize = 8;

constexpr int32_t kInvalidDatabaseId = -1;

enum PlaylistId {
	kDefaultPlaylistId = 1,
	kAlbumPlaylistId,
	kCdPlaylistId,
	kFileSystemPlaylistId,
	kMaxExistPlaylist
};

constexpr auto kYouTubeCategory = "YouTube"_str;
constexpr auto kLocalCategory = "Local"_str;
constexpr auto kHiResCategory = "HiRes"_str;
constexpr auto kDsdCategory = "DSD"_str;

enum PlayingState {
	PLAY_CLEAR = 0,
	PLAY_PLAYING,
	PLAY_PAUSE,
};

enum class StoreType {
	LOCAL_STORE,
	PLAYLIST_LOCAL_STORE,
	CLOUD_STORE,
};

XAMP_WIDGET_SHARED_EXPORT inline bool isCloudStore(StoreType store_type) {
	return store_type == StoreType::CLOUD_STORE;
}

XAMP_WIDGET_SHARED_EXPORT inline bool notAddablePlaylist(const int32_t playlist_id) {
	return playlist_id == kAlbumPlaylistId
		|| playlist_id == kCdPlaylistId
		|| playlist_id == kFileSystemPlaylistId;
}

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

	QSqlDatabase& getDatabase() {
		return db_;
	}

private:
	void createTableIfNotExist();

	QString getVersion() const;

	QString connection_name_;
	QSqlDatabase db_;
	LoggerPtr logger_;
};

class XAMP_WIDGET_SHARED_EXPORT DatabaseFactory final {
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

#define qGuiDb SharedSingleton<Database>::GetInstance()

template <typename Func>
class TransactionScope final {
public:
    explicit TransactionScope(Func&& action)
		: action_(std::forward<Func>(action)) {
		result_ = qGuiDb.transaction();
	}

	~TransactionScope() {
		if (!result_) {
			return;
		}

		try {
			action_();
			qGuiDb.commit();
		}
		catch (...) {
			qGuiDb.rollback();			
		}
	}

private:
	bool result_{ false };
	Func action_;
};

struct Transaction final {
	template <typename Func>
	bool complete(Func&& action) {
		if (!qGuiDb.transaction()) {
			return false;
		}

		try {
			action();
			qGuiDb.commit();
			return true;
		}
		catch (...) {
			qGuiDb.rollback();
			logAndShowMessage(std::current_exception());			
		}
		return false;
	}
};

