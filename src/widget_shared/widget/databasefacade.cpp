#include <widget/databasefacade.h>
#include <widget/filesystemworker.h>

#include <execution>

#include <qguiapplication.h>

#include <base/base.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>

#include <metadata/imetadataextractadapter.h>
#include <stream/avfilestream.h>

#include <widget/widget_shared.h>
#include <widget/tagio.h>
#include <widget/http.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/imagecache.h>

XAMP_DECLARE_LOG_NAME(DatabaseFacade);

namespace {
    inline constexpr auto kJop = qTEXT("jpop");
    inline constexpr auto kHiRes = qTEXT("HiRes");
    inline constexpr auto kDsdCategory = qTEXT("DSD");
    const std::wstring kDffFileExtension(L".dff");
    const std::wstring kDsfFileExtension(L".dsf");
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

LocalStorage<UnknownArtistAndAlbumId> DatabaseFacade::unknown_id_;

UnknownArtistAndAlbumId::UnknownArtistAndAlbumId() {
    first = qMainDb.addOrUpdateArtist(QString::fromStdWString(kUnknownArtist));
    second = qMainDb.addOrUpdateAlbum(QString::fromStdWString(kUnknownAlbum), first, 0, 0, StoreType::CLOUD_STORE);
    qMainDb.setAlbumCover(second, qImageCache.unknownCoverId());
}

DatabaseFacade::DatabaseFacade(QObject* parent, Database* database)
    : QObject(parent) {
    logger_ = LoggerManager::GetInstance().GetLogger(kDatabaseFacadeLoggerName);
    if (!database) {
        database_ = &qMainDb;
    } else {
        database_ = database;
    }
}

int32_t DatabaseFacade::unknownArtistId() {
    return unknown_id_->first;
}

int32_t DatabaseFacade::unknownAlbumId() {
    return unknown_id_->second;
}

void DatabaseFacade::addTrackInfo(const ForwardList<TrackInfo>& result, 
    int32_t playlist_id,
    StoreType store_type,
    const std::function<void(int32_t, int32_t)>& fetch_cover) {
    const Stopwatch sw;
    uint32_t album_year = 0;
    if (!result.empty()) {
        album_year = result.front().year;
    }

    auto album_genre = kEmptyString;

	for (const auto& track_info : result) {        
        auto file_path = toQString(track_info.file_path);
        auto album = toQString(track_info.album);
        auto artist = toQString(track_info.artist);
		auto disc_id = toQString(track_info.disc_id);

        QStringList artists;
        NormalizeArtist(artist, artists);
                
        const auto is_file_path = IsFilePath(track_info.file_path);

        QPixmap cover;
		if (is_file_path && album.isEmpty()) {
			const TagIO reader;
			album = qTR("Unknown album");
			// TODO: 如果有內建圖片就把當作一張專輯.
			cover = reader.embeddedCover(track_info);
			if (!cover.isNull()) {
				album = toQString(track_info.file_name_no_ext);
			}
		}

		if (artist.isEmpty()) {
			artist = qTR("Unknown artist");
		}

        const auto music_id = database_->addOrUpdateMusic(track_info);
        XAMP_EXPECTS(music_id != 0);

        const auto artist_id = database_->addOrUpdateArtist(artist);
        XAMP_EXPECTS(artist_id != 0);

        auto album_id = database_->getAlbumId(album);
        if (album_id == kInvalidDatabaseId) {
            if (track_info.file_path.rfind(L'.') != std::wstring::npos) {
                album_genre = kEmptyString; 
            } else {
                album_genre = qTEXT("Youtube");
            }

            album_id = database_->addOrUpdateAlbum(album,
                artist_id,
                track_info.last_write_time,
                album_year,
                store_type,
                disc_id,
                album_genre);

            if (album_genre.isEmpty()) {
                for (const auto& category : GetAlbumCategories(album)) {
                    database_->addOrUpdateAlbumCategory(album_id, category);
                }
            } else {
                database_->addOrUpdateAlbumCategory(album_id, album_genre);
            }

            if (track_info.file_ext == kDsfFileExtension || track_info.file_ext == kDffFileExtension) {
                database_->addOrUpdateAlbumCategory(album_id, kDsdCategory);
            }
            else if (track_info.bit_rate >= k24Bit96KhzBitRate) {
                database_->addOrUpdateAlbumCategory(album_id, kHiRes);
			}            
        }

        XAMP_EXPECTS(album_id != 0);

		if (playlist_id != -1) {
            database_->addMusicToPlaylist(music_id, playlist_id, album_id);
		}

        database_->addOrUpdateAlbumMusic(album_id, artist_id, music_id);
        database_->addOrUpdateAlbumArtist(album_id, artist_id);
        for (const auto& multi_artist : artists) {
	        const auto id = database_->addOrUpdateArtist(multi_artist);
            database_->addOrUpdateAlbumArtist(album_id, id);
        }

        QString cover_id;
        if (album_id != unknownAlbumId()) {
            cover_id = database_->getAlbumCoverId(album_id);
            if (isNullOfEmpty(cover_id)) {
                cover_id = database_->getMusicCoverId(music_id);
            }

            if (!is_file_path) {
                if (fetch_cover != nullptr && isNullOfEmpty(cover_id)) {
                    fetch_cover(music_id, album_id);
                }
                continue;
            }
        } else {
            cover_id = database_->getMusicCoverId(music_id);

            if (fetch_cover != nullptr && isNullOfEmpty(cover_id)) {
                fetch_cover(music_id, album_id);
            }
            continue;
        }

        if (!cover.isNull()) {
            database_->setAlbumCover(album_id, qImageCache.addImage(cover));
            continue;
        }

        if (isNullOfEmpty(cover_id)) {
            if (!fetch_cover) {
                emit findAlbumCover(album_id, track_info.file_path);
            } else {
                fetch_cover(music_id, album_id);
            }
        }
	}
    if (sw.ElapsedSeconds() > 1.0) {
        XAMP_LOG_DEBUG("AddTrackInfo ({} secs)", sw.ElapsedSeconds());
    }
}

void DatabaseFacade::insertTrackInfo(const ForwardList<TrackInfo>& result,
    int32_t playlist_id,
    StoreType store_type,
    const std::function<void(int32_t, int32_t)>& fetch_cover) {
    try {
        if (!database_->transaction()) {
            XAMP_LOG_DEBUG("Failed to begin transaction!");
            return;
        }
        addTrackInfo(result, playlist_id, store_type, fetch_cover);
        if (!database_->commit()) {
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
    database_->rollback();
}
