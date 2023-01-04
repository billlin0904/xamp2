#include <atomic>
#include <utility>
#include <execution>
#include <forward_list>
#include <unordered_set>

#include <QFile>
#include <QDirIterator>
#include <QMap>
#include <QProgressDialog>

#include <base/base.h>
#include <base/str_utilts.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/siphash.h>
#include <base/google_siphash.h>

#include <metadata/api.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadataextractadapter.h>
#include "thememanager.h"

#include <widget/widget_shared.h>
#include <widget/ui_utilts.h>
#include <widget/read_utiltis.h>
#include <widget/http.h>
#include <widget/xmessagebox.h>
#include <widget/database.h>
#include <widget/appsettings.h>
#include <widget/playlisttableview.h>
#include <widget/pixmapcache.h>
#include <widget/metadataextractadapter.h>

using DirPathHash = GoogleSipHash<>;

DatabaseIdCache::DatabaseIdCache()
    : cover_reader_(MakeMetadataReader()) {
}

void DatabaseIdCache::clear() {
    cover_id_cache_.Clear();
    album_id_cache_.Clear();
    artist_id_cache_.Clear();
}

QPixmap DatabaseIdCache::getEmbeddedCover(const TrackInfo& metadata) const {
    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(metadata.file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(),
            static_cast<uint32_t>(buffer.size()));
    }
    return pixmap;
}

QString DatabaseIdCache::addCoverCache(int32_t album_id, const QString& album, const TrackInfo& metadata, bool is_unknown_album) const {
    auto cover_id = qDatabase.getAlbumCoverId(album_id);
    if (!cover_id.isEmpty()) {
    	return cover_id;
    }

    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(metadata.file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(), static_cast<uint32_t>(buffer.size()));
    }
    else {
    	if (!is_unknown_album) {
            pixmap = PixmapCache::findFileDirCover(QString::fromStdWString(metadata.file_path));
    	}          
    }

    if (!pixmap.isNull()) {
        cover_id = qPixmapCache.savePixamp(pixmap);
        XAMP_ASSERT(!cover_id.isEmpty());
        cover_id_cache_.AddOrUpdate(album_id, cover_id);
        qDatabase.setAlbumCover(album_id, album, cover_id);
    }	
    return cover_id;
}

std::tuple<int32_t, int32_t, QString> DatabaseIdCache::addOrGetAlbumAndArtistId(int64_t dir_last_write_time,
    const QString &album,
    const QString &artist, 
    bool is_podcast,
    const QString& disc_id) const {
#if 1
    int32_t artist_id = 0;
    if (auto const * artist_id_op = this->artist_id_cache_.Find(artist)) {
        artist_id = *artist_id_op;
    }
    else {
        artist_id = qDatabase.addOrUpdateArtist(artist);
        this->artist_id_cache_.AddOrUpdate(artist, artist_id);
    }

    int32_t album_id = 0;
    if (auto const* album_id_op = this->album_id_cache_.Find(album + artist)) {
        album_id = *album_id_op;
    }
    else {
        album_id = qDatabase.addOrUpdateAlbum(album, artist_id, dir_last_write_time, is_podcast, disc_id);
        this->album_id_cache_.AddOrUpdate(album + artist, album_id);
    }

    QString cover_id;
    if (auto const* cover_id_op = this->cover_id_cache_.Find(album_id)) {
        cover_id = *cover_id_op;
    }
#else
    auto artist_id = qDatabase.addOrUpdateArtist(artist);
    auto album_id = qDatabase.addOrUpdateAlbum(album, artist_id, dir_last_write_time, is_podcast, disc_id);
    QString cover_id;
#endif
    return std::make_tuple(album_id, artist_id, cover_id);
}

void ::MetadataExtractAdapter::ScanDirFiles(const QSharedPointer<MetadataExtractAdapter>& adapter, const QString &dir) {
    const auto file_name_filters = getFileNameFilter();

    QDirIterator itr(dir, file_name_filters, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);

    DirPathHash hasher(kDirHashKey1, kDirHashKey2);
    ForwardList<Path> paths;

    while (itr.hasNext()) {
        auto path = toNativeSeparators(itr.next()).toStdWString();
        paths.push_front(path);
        hasher.Update(path);
    }

    if (paths.empty()) {
        paths.push_front(dir.toStdWString());
    }

    const auto path_hash = hasher.GetHash();

    const auto db_hash = qDatabase.getParentPathHash(dir);
    if (db_hash == path_hash) {
        XAMP_LOG_DEBUG("Cache hit hash:{} path: {}", db_hash, String::ToString(dir.toStdWString()));
        emit adapter->fromDatabase(qDatabase.getPlayListEntityFromPathHash(db_hash));
        return;
    }

    auto reader = MakeMetadataReader();

    HashMap<std::wstring, ForwardList<TrackInfo>> album_groups;
    std::for_each(paths.begin(), paths.end(), [&](auto& path) {
        auto track_info = reader->Extract(path);
		album_groups[track_info.album].push_front(std::move(track_info));
    });

    std::for_each(album_groups.begin(), album_groups.end(), [&](auto& tracks) {
        std::for_each(tracks.second.begin(), tracks.second.end(), [&](auto& track) {
            track.parent_path_hash = path_hash;
        });
    
        tracks.second.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
        });
    });

    const DirectoryEntry dir_entry(dir.toStdWString());
    std::for_each(album_groups.begin(), album_groups.end(), [&](auto& tracks) {
        emit adapter->readCompleted(ToTime_t(dir_entry.last_write_time()), tracks.second);
    });
}

::MetadataExtractAdapter::MetadataExtractAdapter(QObject* parent)
    : QObject(parent) {    
}

#define IgnoreSqlException(expr) \
    do {\
		try {\
			expr;\
		}\
		catch (...) {}\
    } while (false)

void ::MetadataExtractAdapter::addMetadata(const ForwardList<TrackInfo>& result, PlayListTableView* playlist, int64_t dir_last_write_time, bool is_podcast) {
	auto playlist_id = -1;
	if (playlist != nullptr) {
		playlist_id = playlist->playlistId();
	}

	for (const auto& track_info : result) {
		auto album = QString::fromStdWString(track_info.album);
		auto artist = QString::fromStdWString(track_info.artist);
		auto disc_id = QString::fromStdString(track_info.disc_id);
     
		auto is_unknown_album = false;
		if (album.isEmpty()) {
			album = tr("Unknown album");
			is_unknown_album = true;
			// todo: 如果有內建圖片就把當作一張專輯.
			auto cover = qDatabaseIdCache.getEmbeddedCover(track_info);
			if (!cover.isNull()) {
				album = QString::fromStdWString(track_info.file_name_no_ext);
			}
		}
		if (artist.isEmpty()) {
			artist = tr("Unknown artist");
		}

		const auto music_id = qDatabase.addOrUpdateMusic(track_info);

		auto [album_id, artist_id, cover_id] = 
            qDatabaseIdCache.addOrGetAlbumAndArtistId(dir_last_write_time,
		             album, 
		             artist,
		             is_podcast, 
		             disc_id);

		if (playlist_id != -1) {
			qDatabase.addMusicToPlaylist(music_id, playlist_id, album_id);
		}

		if (track_info.cover_id.empty()) {
			// Find cover id from database.
			if (cover_id.isEmpty()) {
				cover_id = qDatabase.getAlbumCoverId(album_id);
			}

			// Database not exist find others.
			if (cover_id.isEmpty()) {
				cover_id = qDatabaseIdCache.addCoverCache(album_id, album, track_info, is_unknown_album);
			}
		} else {
			qDatabase.setAlbumCover(album_id, album, QString::fromStdString(track_info.cover_id));
		}

        IgnoreSqlException(qDatabase.addOrUpdateAlbumMusic(album_id,
		                                artist_id,
		                                music_id));
	}

	if (playlist != nullptr) {
		playlist->executeQuery();
	}
}

void ::MetadataExtractAdapter::processMetadata(const ForwardList<TrackInfo>& result, PlayListTableView* playlist, int64_t dir_last_write_time) {
    // Note: Don't not call qApp->processEvents(), maybe stack overflow issue.

    if (dir_last_write_time == -1) {
        dir_last_write_time = QDateTime::currentSecsSinceEpoch();
    }
    auto is_podcast_mode = false;
    if (playlist != nullptr) {
        is_podcast_mode = playlist->isPodcastMode();
    }
    try {
        qDatabase.transaction();
        addMetadata(result, playlist, dir_last_write_time, is_podcast_mode);
        qDatabase.commit();
    } catch (std::exception const &e) {
        qDatabase.rollback();
        XAMP_LOG_DEBUG("processMetadata throw exception! {}", e.what());
    }
}

TrackInfo getMetadata(QString const& file_path) {
    const Path path(file_path.toStdWString());
    auto reader = MakeMetadataReader();
    return reader->Extract(path);
}

QString getFileDialogFileExtensions() {
    QString exts(qTEXT("("));
    for (const auto& file_ext : GetSupportFileExtensions()) {
        exts += qTEXT("*") + QString::fromStdString(file_ext);
        exts += qTEXT(" ");
    }
    exts += qTEXT(")");
    return exts;
}

QStringList getFileNameFilter() {
    QStringList name_filter;
    for (auto& file_ext : GetSupportFileExtensions()) {
        name_filter << qSTR("*%1").arg(QString::fromStdString(file_ext));
    }
    return name_filter;
}
