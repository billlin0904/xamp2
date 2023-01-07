#include <atomic>
#include <utility>
#include <execution>
#include <forward_list>

#include <QFile>
#include <QDirIterator>

#include <base/base.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>

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

DatabaseIdCache::DatabaseIdCache()
    : cover_reader_(MakeMetadataReader()) {
}

QPixmap DatabaseIdCache::getEmbeddedCover(const TrackInfo& track_info) const {
    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(track_info.file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(),
            static_cast<uint32_t>(buffer.size()));
    }
    return pixmap;
}

QString DatabaseIdCache::addCoverCache(int32_t album_id, const QString& album, const TrackInfo& track_info, bool is_unknown_album) const {
    auto cover_id = qDatabase.getAlbumCoverId(album_id);
    if (!cover_id.isEmpty()) {
    	return cover_id;
    }

    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(track_info.file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(), static_cast<uint32_t>(buffer.size()));
    }
    else {
    	if (!is_unknown_album) {
            pixmap = PixmapCache::findFileDirCover(QString::fromStdWString(track_info.file_path));
    	}          
    }

    if (!pixmap.isNull()) {
        cover_id = qPixmapCache.savePixamp(pixmap);
        XAMP_ASSERT(!cover_id.isEmpty());
        qDatabase.setAlbumCover(album_id, album, cover_id);
    }
    return cover_id;
}

std::tuple<int32_t, int32_t> DatabaseIdCache::addOrGetAlbumAndArtistId(int64_t dir_last_write_time,
    const QString &album,
    const QString &artist, 
    bool is_podcast,
    const QString& disc_id) {
    auto artist_id = qDatabase.addOrUpdateArtist(artist);
    auto album_id = qDatabase.addOrUpdateAlbum(album, artist_id, dir_last_write_time, is_podcast, disc_id);
    return std::make_tuple(album_id, artist_id);
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

void ::MetadataExtractAdapter::addMetadata(const ForwardList<TrackInfo>& result,
    int32_t playlist_id,
    int64_t dir_last_write_time, 
    bool is_podcast) {
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

		auto [album_id, artist_id] = 
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
            auto cover_id = qDatabase.getAlbumCoverId(album_id);
			// Database not exist find others.
			if (cover_id.isEmpty()) {
				cover_id = qDatabaseIdCache.addCoverCache(album_id, album, track_info, is_unknown_album);
			}
		} else {
			qDatabase.setAlbumCover(album_id, album, QString::fromStdString(track_info.cover_id));
		}

        IgnoreSqlException(qDatabase.addOrUpdateAlbumMusic(album_id, artist_id, music_id));
	}
}

void ::MetadataExtractAdapter::processMetadata(const ForwardList<TrackInfo>& result, int64_t dir_last_write_time, int32_t playlist_id,  bool is_podcast_mode) {
    // Note: Don't not call qApp->processEvents(), maybe stack overflow issue.

    if (dir_last_write_time == -1) {
        dir_last_write_time = QDateTime::currentSecsSinceEpoch();
    }

    try {
        qDatabase.transaction();
        addMetadata(result, playlist_id, dir_last_write_time, is_podcast_mode);
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
