#include <widget/databasefacade.h>
#include <widget/worker/filesystemworker.h>

#include <qguiapplication.h>

#include <base/base.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>

#include <stream/avfilestream.h>

#include <widget/util/str_util.h>
#include <widget/widget_shared.h>
#include <widget/tagio.h>
#include <widget/http.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/imagecache.h>

XAMP_DECLARE_LOG_NAME(DatabaseFacade);

namespace {
    const std::wstring kVariousArtists(L"Various Artists");
    const std::wstring kDffFileExtension(L".dff");
    const std::wstring kDsfFileExtension(L".dsf");
    constexpr auto k24Bit96KhzBitRate = 4608;

    QSet<QString> getAlbumCategories(const QString& album) {
        static const QStringList categoriesList = {
        qTEXT("piano"), 
        qTEXT("vocal"), 
        qTEXT("soundtrack"), 
        qTEXT("best"),
        qTEXT("complete"),
        qTEXT("collection"), 
        qTEXT("collections"),
        qTEXT("edition"),
        qTEXT("version")
        };

        QSet<QString> categories;
        for (const QString& category : categoriesList) {
            QRegularExpression regex(qSTR("\\b%1\\b").arg(QRegularExpression::escape(category)), QRegularExpression::CaseInsensitiveOption);
            if (regex.match(album).hasMatch()) {
                categories.insert(category.toLower().trimmed());
            }
        }
        return categories;
    }

    QStringList normalizeArtist(QString artist) {
        QStringList artists;
        // Trim whitespace from the artist name
        artist = artist.trimmed();

        // If the artist name contains spaces and is not fully capitalized, remove spaces
        if (artist.contains(qTEXT(" "))) {
            if (!artist[0].isUpper() || !artist.toUpper().contains(artist)) {
                artist = artist.remove(qTEXT(" "));
            }
        }

        // Split artist names by common delimiters
        artists = artist.split(QRegularExpression(qTEXT("([,/&])")), Qt::SkipEmptyParts);

        // Standardize the first artist name and update the list
        if (!artists.isEmpty()) {
            artist = artists.first().trimmed();
            artists.removeFirst();
        }
        return artists;
    }
}

DatabaseFacade::DatabaseFacade(QObject* parent, Database* database)
    : QObject(parent) {
    logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(DatabaseFacade));
    if (!database) {
        database_ = &qGuiDb;
    } else {
        database_ = database;
    }
    initialUnknownTranslateString();
    ensureAddUnknownId();
}

void DatabaseFacade::initialUnknownTranslateString() {
    unknown_         = tr(QT_TRANSLATE_NOOP("DatabaseFacade", "Unknown"));
    unknown_artist_  = tr(QT_TRANSLATE_NOOP("DatabaseFacade", "Unknown artist"));
    unknown_album_   = tr(QT_TRANSLATE_NOOP("DatabaseFacade", "Unknown album"));
    various_artists_ = tr(QT_TRANSLATE_NOOP("DatabaseFacade", "Various Artists"));
}

void DatabaseFacade::ensureAddUnknownId() {
    kVariousArtistsId = qGuiDb.addOrUpdateArtist(various_artists_);
    kUnknownArtistId = qGuiDb.addOrUpdateArtist(unknown_artist_);
    kUnknownAlbumId =  qGuiDb.addOrUpdateAlbum(unknown_album_,
        kUnknownArtistId,
        0,
        0,
        StoreType::CLOUD_STORE);
    database_->addAlbumCategory(kUnknownAlbumId, kLocalCategory);
    qGuiDb.setAlbumCover(kUnknownAlbumId, qImageCache.unknownCoverId());
}

int32_t DatabaseFacade::unknownArtistId() const {
    return kUnknownArtistId;
}

int32_t DatabaseFacade::unknownAlbumId() const {
    return kUnknownAlbumId;
}

void DatabaseFacade::addTrackInfo(const ForwardList<TrackInfo>& result, 
    int32_t playlist_id,
    StoreType store_type,
    const std::function<void(int32_t, int32_t)>& fetch_cover) {
    const Stopwatch sw;

    ensureAddUnknownId();

	for (const auto& track_info : result) {        
        auto file_path = toQString(track_info.file_path);
        auto album     = toQString(track_info.album).trimmed();
        auto artist    = toQString(track_info.artist).trimmed();
		auto disc_id   = toQString(track_info.disc_id);

        if (album.isEmpty()) {
            album = unknown_album_;
        }

		if (artist.isEmpty()) {
			artist = unknown_artist_;
		}

        const auto music_id = database_->addOrUpdateMusic(track_info);
        XAMP_EXPECTS(music_id != 0);

        auto artist_id = database_->addOrUpdateArtist(artist);
        XAMP_EXPECTS(artist_id != 0);

        auto album_id = kInvalidDatabaseId;
        if (isCloudStore(store_type)) {
            album_id = database_->getAlbumId(album);
        }
        else {
            // Avoid cue file album name create new album id.
            album_id = database_->getAlbumIdFromAlbumMusic(music_id);
        }

        if (album_id == kInvalidDatabaseId) {
            auto is_hires = track_info.bit_rate >= k24Bit96KhzBitRate;
            album_id = database_->addOrUpdateAlbum(album,
                artist_id,
                track_info.last_write_time,
                track_info.year,
                store_type,
                disc_id,
                is_hires);

            if (isCloudStore(store_type)) {
                database_->addAlbumCategory(album_id, kYouTubeCategory);
            }
            else {
                database_->addAlbumCategory(album_id, kLocalCategory);
            }

            if (track_info.file_ext() == kDsfFileExtension || track_info.file_ext() == kDffFileExtension) {
                database_->addAlbumCategory(album_id, kDsdCategory);
            }

            if (is_hires) {
                database_->addAlbumCategory(album_id, kHiResCategory);
			}

            for (const auto& category : getAlbumCategories(album)) {
                database_->addAlbumCategory(album_id, category);
            }
        }

        XAMP_EXPECTS(album_id != 0);

		if (playlist_id != kInvalidDatabaseId) {
            database_->addMusicToPlaylist(music_id, playlist_id, album_id);
		}

        database_->addOrUpdateAlbumMusic(album_id, artist_id, music_id);
        database_->addOrUpdateAlbumArtist(album_id, artist_id);

        if (artist_id != kUnknownArtistId) {
            for (const auto& artist : normalizeArtist(artist)) {
                const auto id = database_->addOrUpdateArtist(artist);
                database_->addOrUpdateAlbumArtist(album_id, id);
            }
        }

        QString cover_id;
        if (album_id != unknownAlbumId()) {
            cover_id = database_->getAlbumCoverId(album_id);
            if (isNullOfEmpty(cover_id)) {
                cover_id = database_->getMusicCoverId(music_id);
            }
        } else {
            cover_id = database_->getMusicCoverId(music_id);
        }

        if (isNullOfEmpty(cover_id)) {
            if (fetch_cover != nullptr) {
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
    TransactionScope scope([&]() {
        addTrackInfo(result, playlist_id, store_type, fetch_cover);
        });
}
