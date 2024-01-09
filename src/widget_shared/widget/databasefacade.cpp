#include <widget/databasefacade.h>
#include <widget/extractfileworker.h>

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

DatabaseFacade::DatabaseFacade(QObject* parent, Database* database)
    : QObject(parent) {
    logger_ = LoggerManager::GetInstance().GetLogger(kDatabaseFacadeLoggerName);
    if (!database) {
        database_ = &qMainDb;
    } else {
        database_ = database;
    }
}

void DatabaseFacade::initUnknownAlbumAndArtist() {
    if (unknown_album_id_ != kInvalidDatabaseId) {
        return;
    }
    unknown_artist_id_ = database_->addOrUpdateArtist(QString::fromStdWString(kUnknownArtist));
    unknown_album_id_ = database_->addOrUpdateAlbum(QString::fromStdWString(kUnknownAlbum), unknown_artist_id_, -1, -1, StoreType::CLOUD_STORE);
    database_->setAlbumCover(unknown_album_id_, qImageCache.unknownCoverId());
}

void DatabaseFacade::addTrackInfo(const ForwardList<TrackInfo>& result, 
    int32_t playlist_id,
    StoreType store_type,
    std::function<void(int32_t)> fetch_cover) {
    const Stopwatch sw;
    uint32_t album_year = 0;
    if (!result.empty()) {
        album_year = result.front().year;
    }

    constexpr auto album_genre = kEmptyString;

    initUnknownAlbumAndArtist();

	for (const auto& track_info : result) {        
        auto file_path = getStringOrEmptyString(track_info.file_path);
        auto album = getStringOrEmptyString(track_info.album);
        auto artist = getStringOrEmptyString(track_info.artist);
		auto disc_id = getStringOrEmptyString(track_info.disc_id);

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
				album = getStringOrEmptyString(track_info.file_name_no_ext);
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
            album_id = database_->addOrUpdateAlbum(album,
                artist_id,
                track_info.last_write_time,
                album_year,
                store_type,
                disc_id,
                album_genre);
            for (const auto &category : GetAlbumCategories(album)) {
                database_->addOrUpdateAlbumCategory(album_id, category);
            }
            
            if (track_info.file_ext == kDsfExtension || track_info.file_ext == kDffExtension) {
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

        if (album_id == unknown_album_id_) {
            continue;
        }

        const auto cover_id = database_->getAlbumCoverId(album_id);

        if (!is_file_path) {
            if (fetch_cover != nullptr && cover_id.isEmpty()) {
                fetch_cover(album_id);
            }
            continue;
        }

        if (!cover.isNull()) {
            database_->setAlbumCover(album_id, qImageCache.addImage(cover));
            continue;
        }

        if (cover_id.isEmpty()) {
            if (!fetch_cover) {
                emit findAlbumCover(album_id, track_info.file_path);
            } else {
                fetch_cover(album_id);
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
    std::function<void(int32_t)> fetch_cover) {
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
