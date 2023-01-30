#include <atomic>
#include <utility>
#include <execution>
#include <forward_list>

#include <QFile>
#include <QDirIterator>
#include <QtConcurrent/qtconcurrentrun.h>

#include <base/base.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>

#include <metadata/api.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadataextractadapter.h>
#include "thememanager.h"

#include <widget/widget_shared.h>
#include <widget/http.h>
#include <widget/xmessagebox.h>
#include <widget/database.h>
#include <widget/appsettings.h>
#include <widget/playlisttableview.h>
#include <widget/pixmapcache.h>
#include <widget/metadataextractadapter.h>

#define IGNORE_ANY_EXCEPTION(expr) \
    do {\
		try {\
			expr;\
		}\
		catch (...) {}\
    } while (false)

CoverArtReader::CoverArtReader()
    : cover_reader_(MakeMetadataReader()) {
}

QPixmap CoverArtReader::getEmbeddedCover(const Path& file_path) const {
    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(), static_cast<uint32_t>(buffer.size()));
    }
    return pixmap;
}

QPixmap CoverArtReader::getEmbeddedCover(const TrackInfo& track_info) const {
    return getEmbeddedCover(track_info.file_path);
}

DatabaseProxy::DatabaseProxy(QObject* parent)
    : QObject(parent) {    
}

void DatabaseProxy::findAlbumCover(int32_t album_id, const std::wstring& album, const std::wstring& file_path, const CoverArtReader& reader) {
	const auto cover_id = qDatabase.getAlbumCoverId(album_id);
    if (!cover_id.isEmpty()) {
        return;
    }

    std::wstring find_file_path;
	const auto first_file_path = qDatabase.getAlbumFirstMusicFilePath(album_id);
    if (!first_file_path) {
        find_file_path = file_path;
    } else {
        find_file_path = (*first_file_path).toStdWString();
    }

    auto cover = reader.getEmbeddedCover(find_file_path);
    if (cover.isNull()) {
        cover = PixmapCache::findCoverInDir(QString::fromStdWString(file_path));
    }

    if (!cover.isNull()) {
        qDatabase.setAlbumCover(album_id, QString::fromStdWString(album), qPixmapCache.savePixamp(cover));
    }
}

void DatabaseProxy::addTrackInfo(const ForwardList<TrackInfo>& result,
    int32_t playlist_id,
    int64_t dir_last_write_time, 
    bool is_podcast) {
	const CoverArtReader reader;
    // note: Parameter 'result' must be same album name.    
	for (const auto& track_info : result) {
		auto album = QString::fromStdWString(track_info.album);
		auto artist = QString::fromStdWString(track_info.artist);
		auto disc_id = QString::fromStdString(track_info.disc_id);

        QPixmap cover;
		if (album.isEmpty()) {
			album = tr("Unknown album");
			// todo: 如果有內建圖片就把當作一張專輯.
			cover = reader.getEmbeddedCover(track_info);
			if (!cover.isNull()) {
				album = QString::fromStdWString(track_info.file_name_no_ext);
			}
		}

		if (artist.isEmpty()) {
			artist = tr("Unknown artist");
		}

		const auto music_id = qDatabase.addOrUpdateMusic(track_info);
		const auto artist_id = qDatabase.addOrUpdateArtist(artist);
		const auto album_id = qDatabase.addOrUpdateAlbum(album,
            artist_id,
            dir_last_write_time, 
            is_podcast,
            disc_id);

		if (playlist_id != -1) {
			qDatabase.addMusicToPlaylist(music_id, playlist_id, album_id);
		}

        IGNORE_ANY_EXCEPTION(qDatabase.addOrUpdateAlbumMusic(album_id, artist_id, music_id));

        if (!cover.isNull()) {
            qDatabase.setAlbumCover(album_id, album, qPixmapCache.savePixamp(cover));
        } else {
            findAlbumCover(album_id, track_info.album, track_info.file_path, reader);
        }
	}
}

void DatabaseProxy::insertTrackInfo(const ForwardList<TrackInfo>& result, int32_t playlist_id, bool is_podcast_mode) {
    // Note: Don't not call qApp->processEvents(), maybe stack overflow issue.
    try {
        qDatabase.transaction();
        addTrackInfo(result, playlist_id, QDateTime::currentSecsSinceEpoch(), is_podcast_mode);
        qDatabase.commit();
    } catch (std::exception const &e) {
        qDatabase.rollback();
        XAMP_LOG_DEBUG("insertTrackInfo throw exception! {}", e.what());
    }
}

TrackInfo getTrackInfo(QString const& file_path) {
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
