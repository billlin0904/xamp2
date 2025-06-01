#include <widget/databasefacade.h>
#include <widget/worker/filesystemservice.h>

#include <base/base.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>

#include <widget/util/str_util.h>
#include <widget/widget_shared.h>
#include <widget/dao/artistdao.h>
#include <widget/dao/albumdao.h>
#include <widget/dao/musicdao.h>
#include <widget/dao/playlistdao.h>
#include <widget/playlisttableview.h>
#include <widget/imagecache.h>

XAMP_DECLARE_LOG_NAME(DatabaseFacade);

namespace {
    const std::wstring kVariousArtists(L"Various Artists");
    const std::wstring kDffFileExtension(L".dff");
    const std::wstring kDsfFileExtension(L".dsf");
    constexpr auto k24Bit96KhzBitRate = 4608;

    QSet<QString> getAlbumCategories(const QString& album, dao::AlbumDao &album_dao) {
        const QStringList categoriesList = album_dao.getAlbumTags();

        QSet<QString> categories;
        for (const QString& category : categoriesList) {
            QRegularExpression regex(
                qFormat("\\b%1\\b").arg(QRegularExpression::escape(category)),
                QRegularExpression::CaseInsensitiveOption);
            if (regex.match(album).hasMatch()) {
                categories.insert(category);
            }
        }
        return categories;
    }

    QStringList normalizeArtist(QString artist) {
        QStringList artists;
        // Trim whitespace from the artist name
        artist = artist.trimmed();

        // If the artist name contains spaces and is not fully capitalized, remove spaces
        if (artist.contains(" "_str)) {
            if (!artist[0].isUpper() || !artist.toUpper().contains(artist)) {
                artist = artist.remove(" "_str);
            }
        }

        // Split artist names by common delimiters
        artists = artist.split(QRegularExpression("([,/&])"_str), Qt::SkipEmptyParts);

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
        dao_facade_.reset(new dao::DatabaseFacade(database_));
        initialUnknownTranslateString();
        ensureAddUnknownId();
    } else {
        initialUnknownTranslateString();
        database_ = database;
        dao_facade_.reset(new dao::DatabaseFacade(database_));
    }    
}

DatabaseFacade::~DatabaseFacade() = default;

void DatabaseFacade::initialUnknownTranslateString() {
    unknown_         = tr(QT_TRANSLATE_NOOP("DatabaseFacade", "Unknown"));
    unknown_artist_  = tr(QT_TRANSLATE_NOOP("DatabaseFacade", "Unknown artist"));
    unknown_album_   = tr(QT_TRANSLATE_NOOP("DatabaseFacade", "Unknown album"));
    various_artists_ = tr(QT_TRANSLATE_NOOP("DatabaseFacade", "Various Artists"));
}

void DatabaseFacade::ensureAddUnknownId() {
    dao::ArtistDao artist(database_->getDatabase());
    dao::AlbumDao album(database_->getDatabase());

    kVariousArtistsId = artist.addOrUpdateArtist(various_artists_);
    kUnknownArtistId  = artist.addOrUpdateArtist(unknown_artist_);
    kUnknownAlbumId   = album.addOrUpdateAlbum(unknown_album_,
        kUnknownArtistId,
        0,
        0,
        StoreType::PLAYLIST_LOCAL_STORE);
    album.addAlbumCategory(kUnknownAlbumId, kLocalCategory);
    album.setAlbumCover(kUnknownAlbumId, qImageCache.unknownCoverId());
}

int32_t DatabaseFacade::unknownArtistId() const {
    return kUnknownArtistId;
}

int32_t DatabaseFacade::unknownAlbumId() const {
    return kUnknownAlbumId;
}

void DatabaseFacade::insertTrackInfo(const std::forward_list<TrackInfo>& result, 
    int32_t playlist_id,
    StoreType store_type,
    const QString& dick_id,
    const std::function<void(int32_t, int32_t)>& fetch_cover) {
    if (result.empty()) {
        return;
    }
    
    ensureAddUnknownId();

    auto count_artist = 0;
    const auto& front = result.front();
    for (const auto& track_info : result) {
		if (front.artist == track_info.artist) {
			count_artist++;
            break;
		}
    }

	for (const auto& track_info : result) {        
        auto file_path = toQString(track_info.file_path);
        auto album     = toQString(track_info.album).trimmed();
        auto artist    = toQString(track_info.artist).trimmed();
		auto disc_id   = toQString(track_info.disc_id);

        if (store_type != StoreType::CLOUD_STORE
            && dao_facade_->music_dao.getMusicId(file_path)
            && playlist_id == kFileSystemPlaylistId) {
            continue;
        }

        if (album.isEmpty()) {
            album = unknown_album_;
        }

		if (artist.isEmpty()) {
			artist = unknown_artist_;
		}

        const auto music_id = dao_facade_->music_dao.addOrUpdateMusic(track_info);
        XAMP_EXPECTS(music_id != 0);

		auto artist_id = dao_facade_->artist_dao.getArtistId(artist);

        if (artist_id == kInvalidDatabaseId) {
            if (count_artist > 1) {
                artist = QString::fromStdWString(kVariousArtists);
            }
            artist_id = dao_facade_->artist_dao.addOrUpdateArtist(artist,
                artist.left(1).toUpper());
            XAMP_EXPECTS(artist_id != 0);
        }

        auto album_id = kInvalidDatabaseId;
        if (isCloudStore(store_type)) {
            album_id = dao_facade_->album_dao.getAlbumId(album);
        }
        else {
            // Avoid cue file album name create new album id.
            album_id = dao_facade_->album_dao.getAlbumIdFromAlbumMusic(music_id);
        }

        if (album_id == kInvalidDatabaseId) {
            auto is_hires = track_info.bit_rate >= k24Bit96KhzBitRate;

            album_id = dao_facade_->album_dao.addOrUpdateAlbum(album,
                artist_id,
                track_info.last_write_time,
                track_info.year,
                store_type,
                disc_id,
                is_hires);

            if (isCloudStore(store_type)) {
                dao_facade_->album_dao.addAlbumCategory(album_id, kYouTubeCategory);
            }
            else {
                dao_facade_->album_dao.addAlbumCategory(album_id, kLocalCategory);
                auto disk = "Disk("_str +
                    QString::fromStdWString(track_info.file_path.root_name().wstring()) + ")"_str;
                dao_facade_->album_dao.addAlbumCategory(album_id, disk);
            }

            if (track_info.file_ext() == kDsfFileExtension 
                || track_info.file_ext() == kDffFileExtension) {
                dao_facade_->album_dao.addAlbumCategory(album_id, kDsdCategory);
            }

            if (is_hires) {
                dao_facade_->album_dao.addAlbumCategory(album_id, kHiResCategory);
            }

            for (const auto& category : getAlbumCategories(album, dao_facade_->album_dao)) {
                dao_facade_->album_dao.addAlbumCategory(album_id, category);
            }
        }

        XAMP_EXPECTS(album_id > 0);

		if (playlist_id != kInvalidDatabaseId) {
            dao_facade_->playlist_dao.addMusicToPlaylist(music_id, playlist_id, album_id);
		}

        dao_facade_->album_dao.addOrUpdateAlbumMusic(album_id, artist_id, music_id);
        dao_facade_->album_dao.addOrUpdateAlbumArtist(album_id, artist_id);
        if (!disc_id.isEmpty()) {
            dao_facade_->album_dao.updateAlbumByDiscId(disc_id, album_id);
        }        

        if (artist_id != kUnknownArtistId) {
            for (const auto& artist : normalizeArtist(artist)) {
                const auto id = dao_facade_->artist_dao.addOrUpdateArtist(artist);
                dao_facade_->album_dao.addOrUpdateAlbumArtist(album_id, id);
            }
        }

        QString cover_id;
        if (album_id != unknownAlbumId()) {
            cover_id = dao_facade_->album_dao.getAlbumCoverId(album_id);
        }
        if (isNullOfEmpty(cover_id)) {
            cover_id = dao_facade_->music_dao.getMusicCoverId(music_id);
        }

        if (isNullOfEmpty(cover_id)) {
            if (fetch_cover != nullptr) {
                fetch_cover(music_id, album_id);
            }
        }
	}
}

void DatabaseFacade::insertMultipleTrackInfo(
    const std::vector<std::forward_list<TrackInfo>>& results,
    int32_t playlist_id,
    StoreType store_type,
    const QString& dick_id,
    const std::function<void(int32_t, int32_t)>& fetch_cover) {
    TransactionScope scope([&]() {
        for (const auto& result : results) {
            insertTrackInfo(result,
                playlist_id,
                store_type,
                dick_id,
                fetch_cover);
        }
    }, database_);
}
