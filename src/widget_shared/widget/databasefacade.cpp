#include <widget/databasefacade.h>
#include <widget/extractfileworker.h>

#include <execution>

#include <qguiapplication.h>

#include <base/base.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>

#include <metadata/api.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadataextractadapter.h>
#include <stream/avfilestream.h>

#include <widget/widget_shared.h>
#include <widget/http.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/imagecache.h>

XAMP_DECLARE_LOG_NAME(DatabaseFacade);

namespace {
    constexpr auto kJop = qTEXT("jpop");
    constexpr auto kHiRes = qTEXT("HiRes");
    constexpr auto kDsdCategory = qTEXT("DSD");
    const std::wstring kDffExtension(L".dff");
    const std::wstring kDsfExtension(L".dsf");
    constexpr auto k24Bit96KhzBitRate = 4608;

    QSet<QString> GetAlbumCategories(const QString& album) {
	    const QRegularExpression regex(
            R"((piano|vocal|soundtrack|best|complete|collection|edition|version)(?:(?: \[.*\])|(?: - .*))?)",
            QRegularExpression::CaseInsensitiveOption);

        QSet<QString> categories;
        auto it = regex.globalMatch(album);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString category = match.captured(1).toLower().trimmed();
            categories.insert(category);
        }
        return categories;
    }

    void NormalizeArtist(QString& artist, QStringList& artists) {
        if (artist.contains(' ')) {
            if (!artist[0].isUpper()) {
                artist = artist.remove(' ');
            }
            else {
                artist = artist.trimmed();
            }
        }

        artists = artist.split(QRegularExpression("[,/&]"), Qt::SkipEmptyParts);
        if (!artists.isEmpty()) {
            artist = artists.first();
            artists.pop_front();
        }
    }
}

TagIO::TagIO()
    : reader_(MakeMetadataReader())
    , writer_(MakeMetadataWriter()) {
}

void TagIO::WriteArtist(const Path& path, const QString& artist) {
    writer_->WriteArtist(path, artist.toStdWString());
}

void TagIO::WriteAlbum(const Path& path, const QString& album) {
    writer_->WriteAlbum(path, album.toStdWString());
}

void TagIO::WriteTitle(const Path& path, const QString& title) {
    writer_->WriteTitle(path, title.toStdWString());
}

void TagIO::WriteTrack(const Path& path, uint32_t track) {
    writer_->WriteTrack(path, track);
}

void TagIO::WriteGenre(const Path& path, const QString& genre) {
    writer_->WriteGenre(path, genre.toStdWString());
}

void TagIO::WriteComment(const Path& path, const QString& comment) {
    writer_->WriteComment(path, comment.toStdWString());
}

void TagIO::WriteYear(const Path& path, uint32_t year) {
    writer_->WriteYear(path, year);
}

bool TagIO::GetEmbeddedCover(const Path& file_path, QPixmap& image, size_t& image_size) const {
    const auto& buffer = reader_->ReadEmbeddedCover(file_path);
    image_size = 0;
    if (!buffer.empty()) {
        image.loadFromData(buffer.data(), buffer.size());
        image_size = buffer.size();
        return true;
    }    
    return false;
}

QPixmap TagIO::GetEmbeddedCover(const Path& file_path) const {
    QPixmap pixmap;
    const auto& buffer = reader_->ReadEmbeddedCover(file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(), buffer.size());
    }
    return pixmap;
}

void TagIO::RemoveEmbeddedCover(const Path& file_path) {
    writer_->RemoveEmbeddedCover(file_path);
}

bool TagIO::CanWriteEmbeddedCover(const Path& path) const {
    return writer_->CanWriteEmbeddedCover(path);
}

void TagIO::WriteEmbeddedCover(const Path& file_path, const QPixmap& image) {
    if (image.isNull()) {
        return;
    }

    const auto buffer = image_utils::Image2ByteVector(image);
    writer_->WriteEmbeddedCover(file_path, buffer);
}

QPixmap TagIO::GetEmbeddedCover(const TrackInfo& track_info) const {
    return GetEmbeddedCover(track_info.file_path);
}

DatabaseFacade::DatabaseFacade(QObject* parent)
    : QObject(parent) {
    logger_ = LoggerManager::GetInstance().GetLogger(kDatabaseFacadeLoggerName);    
}

void DatabaseFacade::AddTrackInfo(const ForwardList<TrackInfo>& result, int32_t playlist_id) {
    const Stopwatch sw;
    FloatMap<QString, int32_t> artist_id_cache;
    FloatMap<QString, int32_t> album_id_cache;

    const auto album_year = result.front().year;
    constexpr auto album_genre = kEmptyString;
    
	for (const auto& track_info : result) {        
        auto file_path = GetStringOrEmptyString(track_info.file_path);
        auto album = GetStringOrEmptyString(track_info.album);
        auto artist = GetStringOrEmptyString(track_info.artist);
		auto disc_id = GetStringOrEmptyString(track_info.disc_id);

        QStringList artists;
        NormalizeArtist(artist, artists);
                
        const auto is_file_path = IsFilePath(track_info.file_path);

        QPixmap cover;
		if (is_file_path && album.isEmpty()) {
			const TagIO reader;
			album = tr("Unknown album");
			// todo: 如果有內建圖片就把當作一張專輯.
			cover = reader.GetEmbeddedCover(track_info);
			if (!cover.isNull()) {
				album = GetStringOrEmptyString(track_info.file_name_no_ext);
			}
		}

		if (artist.isEmpty()) {
			artist = tr("Unknown artist");
		}

        const auto music_id = qMainDb.AddOrUpdateMusic(track_info);
        int32_t artist_id = 0;
        if (!artist_id_cache.contains(artist)) {
            artist_id = qMainDb.AddOrUpdateArtist(artist);
            artist_id_cache[artist] = artist_id;
        } else {
            artist_id = artist_id_cache[artist];
        }

        XAMP_EXPECTS(artist_id != 0);

        auto album_id = qMainDb.GetAlbumId(album);
        if (album_id == kInvalidDatabaseId) {
            album_id = qMainDb.AddOrUpdateAlbum(album,
                artist_id,
                track_info.last_write_time,
                album_year,
                false,
                disc_id,
                album_genre);
            album_id_cache[album] = album_id;
            for (const auto &category : GetAlbumCategories(album)) {
                qMainDb.AddOrUpdateAlbumCategory(album_id, category);
            }
            
            if (track_info.file_ext == kDsfExtension || track_info.file_ext == kDffExtension) {
                qMainDb.AddOrUpdateAlbumCategory(album_id, kDsdCategory);
            }
            else if (track_info.bit_rate >= k24Bit96KhzBitRate) {
                qMainDb.AddOrUpdateAlbumCategory(album_id, kHiRes);
			}            
        }

        XAMP_EXPECTS(album_id != 0);

		if (playlist_id != -1) {
            qMainDb.AddMusicToPlaylist(music_id, playlist_id, album_id);
		}

        qMainDb.AddOrUpdateAlbumMusic(album_id, artist_id, music_id);
        qMainDb.AddOrUpdateAlbumArtist(album_id, artist_id);
        for (const auto& multi_artist : artists) {
            auto id = qMainDb.AddOrUpdateArtist(multi_artist);
            qMainDb.AddOrUpdateAlbumArtist(album_id, id);
        }

        if (!is_file_path) {
            continue;
        }

        if (!cover.isNull()) {
            qMainDb.SetAlbumCover(album_id, album, qPixmapCache.AddImage(cover));
            continue;
        }

        const auto cover_id = qMainDb.GetAlbumCoverId(album_id);
        if (cover_id.isEmpty()) {
            //XAMP_LOG_DEBUG("Found album {} cover", album.toStdString());
            emit FindAlbumCover(album_id, album, track_info.file_path);
        }
	}
    if (sw.ElapsedSeconds() > 1.0) {
        XAMP_LOG_DEBUG("AddTrackInfo ({} secs)", sw.ElapsedSeconds());
    }
}

void DatabaseFacade::InsertTrackInfo(const ForwardList<TrackInfo>& result, int32_t playlist_id) {
    try {
        if (!qMainDb.transaction()) {
            XAMP_LOG_DEBUG("Failed to begin transaction!");
            return;
        }
        AddTrackInfo(result, playlist_id);       
        if (!qMainDb.commit()) {
            XAMP_LOG_DEBUG("Failed to commit!");
        }
        return;
    }
    catch (Exception const& e) {
        XAMP_LOG_DEBUG("Failed to add track info({})!", e.GetErrorMessage());
    }
    catch (std::exception const& e) {
        XAMP_LOG_DEBUG("Failed to add track info({})!", e.what());
    }
    catch (...) {
        XAMP_LOG_DEBUG("Failed to add track info!");
    }
    qMainDb.rollback();
}

TrackInfo TagIO::GetTrackInfo(const Path& path) {
    const auto reader = MakeMetadataReader();
    return reader->Extract(path);
}
