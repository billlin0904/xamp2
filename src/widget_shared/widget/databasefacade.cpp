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
