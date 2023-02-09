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
#include <widget/databasefacade.h>

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

QPixmap CoverArtReader::GetEmbeddedCover(const Path& file_path) const {
    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(), static_cast<uint32_t>(buffer.size()));
    }
    return pixmap;
}

QPixmap CoverArtReader::GetEmbeddedCover(const TrackInfo& track_info) const {
    return GetEmbeddedCover(track_info.file_path);
}

LruCache<QString, int32_t> DatabaseFacade::artist_id_cache_;
LruCache<QString, int32_t> DatabaseFacade::album_id_cache_;

DatabaseFacade::DatabaseFacade(QObject* parent)
    : QObject(parent) {    
}

void DatabaseFacade::FindAlbumCover(int32_t album_id, const std::wstring& album, const std::wstring& file_path, const CoverArtReader& reader) {
	const auto cover_id = qDatabase.GetAlbumCoverId(album_id);
    if (!cover_id.isEmpty()) {
        return;
    }

    std::wstring find_file_path;
	const auto first_file_path = qDatabase.GetAlbumFirstMusicFilePath(album_id);
    if (!first_file_path) {
        find_file_path = file_path;
    } else {
        find_file_path = (*first_file_path).toStdWString();
    }

    auto cover = reader.GetEmbeddedCover(find_file_path);
    if (cover.isNull()) {
        cover = PixmapCache::FindCoverInDir(QString::fromStdWString(file_path));
    }

    if (!cover.isNull()) {
        qDatabase.SetAlbumCover(album_id, QString::fromStdWString(album), qPixmapCache.SavePixamp(cover));
    }
}

void DatabaseFacade::AddTrackInfo(const ForwardList<TrackInfo>& result,
    int32_t playlist_id,
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
			cover = reader.GetEmbeddedCover(track_info);
			if (!cover.isNull()) {
				album = QString::fromStdWString(track_info.file_name_no_ext);
			}
		}

		if (artist.isEmpty()) {
			artist = tr("Unknown artist");
		}

        const auto music_id = qDatabase.AddOrUpdateMusic(track_info);
        const auto artist_id = qDatabase.AddOrUpdateArtist(artist);
        const auto album_id = qDatabase.AddOrUpdateAlbum(album,
            artist_id,
            track_info.last_write_time,
            is_podcast,
            disc_id);

		if (playlist_id != -1) {
			qDatabase.AddMusicToPlaylist(music_id, playlist_id, album_id);
		}

        IGNORE_ANY_EXCEPTION(qDatabase.AddOrUpdateAlbumMusic(album_id, artist_id, music_id));

        if (!cover.isNull()) {
            qDatabase.SetAlbumCover(album_id, album, qPixmapCache.SavePixamp(cover));
        } else {
            FindAlbumCover(album_id, track_info.album, track_info.file_path, reader);
        }
	}
}

void DatabaseFacade::InsertTrackInfo(const ForwardList<TrackInfo>& result, int32_t playlist_id, bool is_podcast_mode) {
    // Note: Don't not call qApp->processEvents(), maybe stack overflow issue.
    bool failed_insert_database = false;

    for (auto i = 0; i < 3; ++i) {
        try {
            qDatabase.transaction();
            AddTrackInfo(result, playlist_id, is_podcast_mode);
            qDatabase.commit();
        }
        catch (Exception const& e) {
            failed_insert_database = true;
            XAMP_LOG_DEBUG("Failed to add track info({})!", e.GetErrorMessage());
        }
        catch (std::exception const& e) {
            failed_insert_database = true;
            XAMP_LOG_DEBUG("Failed to add track info({})!", e.what());
        }
        catch (...) {
            failed_insert_database = true;
            XAMP_LOG_DEBUG("Failed to add track info!");
        }

        if (failed_insert_database) {
            qDatabase.rollback();
            XAMP_LOG_DEBUG("Retry insert database count: {}.", i);
        }
    }
}

TrackInfo GetTrackInfo(QString const& file_path) {
    const Path path(file_path.toStdWString());
    auto reader = MakeMetadataReader();
    return reader->Extract(path);
}

QString GetFileDialogFileExtensions() {
    QString exts(qTEXT("("));
    for (const auto& file_ext : GetSupportFileExtensions()) {
        exts += qTEXT("*") + QString::fromStdString(file_ext);
        exts += qTEXT(" ");
    }
    exts += qTEXT(")");
    return exts;
}

QStringList GetFileNameFilter() {
    QStringList name_filter;
    for (auto& file_ext : GetSupportFileExtensions()) {
        name_filter << qSTR("*%1").arg(QString::fromStdString(file_ext));
    }
    return name_filter;
}
