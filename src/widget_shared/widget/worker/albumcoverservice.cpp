#include <QImageReader>
#include <widget/util/json_util.h>
#include <base/object_pool.h>
#include <widget/util/image_util.h>
#include <widget/databasefacade.h>
#include <widget/worker/albumcoverservice.h>
#include <widget/tagio.h>
#include <widget/dao/albumdao.h>
#include <widget/dao/musicdao.h>
#include <widget/imagecache.h>

AlbumCoverService::AlbumCoverService()
    : database_ptr_(getPooledDatabase(2))
    , nam_(this)
	, http_client_(&nam_, QString(), this) {
}

void AlbumCoverService::cleaup() {
    database_ptr_.reset();
}

void AlbumCoverService::enableFetchThumbnail(bool enable) {
    enable_ = enable;
}

void AlbumCoverService::onFetchArtistThumbnailUrl(int32_t artist_id, const QString& thumbnail_url) {
    if (is_stop_) {
        return;
    }

    if (pending_requests_.contains(thumbnail_url)) {
        return;
    }

    if (enable_) {
        http_client_.setUrl(thumbnail_url);
        http_client_.download().then([thumbnail_url, artist_id, this](const auto& content) {
            QPixmap image;
            if (!image.loadFromData(content)) {
                return;
            }
            auto cover_id = qImageCache.addImage(image, false, false);
            emit setAristThumbnail(artist_id, cover_id);
            pending_requests_.erase(thumbnail_url);
            });
    }
    
    pending_requests_.insert(thumbnail_url);
}

void AlbumCoverService::onFetchYoutubeThumbnailUrl(const QString& video_id, const QString& thumbnail_url) {
    if (is_stop_) {
        return;
    }

    if (pending_requests_.contains(thumbnail_url)) {
        return;
    }

    if (enable_) {
        http_client_.setUrl(thumbnail_url);
        http_client_.download().then([thumbnail_url, video_id, this](const auto& content) {
            QPixmap image;
            if (!image.loadFromData(content)) {
                return;
            }
            qImageCache.addCache(video_id, image);
            pending_requests_.erase(thumbnail_url);
            });
    }
    
    pending_requests_.insert(thumbnail_url);
}

void AlbumCoverService::onFetchThumbnailUrl(const DatabaseCoverId& id, const QString& thumbnail_url) {
    if (is_stop_) {
        return;
    }

    if (pending_requests_.contains(thumbnail_url)) {
        return;
    }

    if (enable_) {
        http_client_.setUrl(thumbnail_url);
        http_client_.download().then([thumbnail_url, id, this](const auto& content) {
            QPixmap image;
            if (!image.loadFromData(content)) {
                return;
            }
            emit setThumbnail(id, qImageCache.addImage(image));
            pending_requests_.erase(thumbnail_url);
            });
    }
	
    pending_requests_.insert(thumbnail_url);
}

void AlbumCoverService::cancelRequested() {
    is_stop_ = true;
}

void AlbumCoverService::mergeUnknownAlbumCover() {
	constexpr auto kMaxCoverCount = 4;

    auto db = database_ptr_->Acquire();
    auto album_id = qDatabaseFacade.unknownAlbumId();

    dao::AlbumDao album_dao(db->getDatabase());
    auto album_state = album_dao.getAlbumStats(album_id);
    if (!album_state) {
		return;
    }

    if (album_state.value().songs < kMaxCoverCount) {
        return;
    }

    QList<int32_t> music_ids;
    QList<QPixmap> covers;
    album_dao.forEachAlbumMusic(album_id, [&](const auto& entity) {
        if (covers.size() == kMaxCoverCount) {
            return;
        }
        TagIO tag_io;
		tag_io.Open(entity.file_path.toStdWString(), TAG_IO_READ_MODE);
        try {
            auto image = tag_io.embeddedCover();
            if (!image.isNull()) {
                covers.push_back(image);
                music_ids.append(entity.music_id);
            }
        }
		catch (...) {
			// Ignore exception.
        }
        });
    
    if (covers.size() < kMaxCoverCount) {
        return;
    }

    auto image = image_util::mergeImage(covers);
    auto cover_id = qImageCache.addImage(image);

    dao::MusicDao music_dao(db->getDatabase());
    TransactionScope scope([&]() {
        album_dao.forEachAlbumMusic(album_id, [&](const auto& entity) {
            music_dao.setMusicCover(entity.music_id, cover_id);
            });
        });
}

void AlbumCoverService::onFindAlbumCover(const DatabaseCoverId& id) {
    is_stop_ = false;

    if (!enable_) {
        return;
    }

    auto db = database_ptr_->Acquire();
    dao::AlbumDao album_dao(db->getDatabase());
    dao::MusicDao music_dao(db->getDatabase());

    try {
	    const auto cover_id = album_dao.getAlbumCoverId(id.second.value());
        if (!isNullOfEmpty(cover_id) && cover_id != qImageCache.unknownCoverId()) {
			emit setAlbumCover(id.second.value(), cover_id);
            return;
        }

        // 1. Read embedded cover in music file.
        auto music_file_path = music_dao.getMusicFilePath(id.first).toStdWString();
        
        if (music_file_path.empty()) {
            // 2. Read embedded cover in album first music file.
            if (auto file_path = album_dao.getAlbumFirstMusicFilePath(id.second.value())) {
                music_file_path = file_path->toStdWString();
            }            
        }

        // 3. Read file embedded cover.
        if (music_file_path.empty()) {
            return;
        }

        if (!IsFilePath(music_file_path)) {
            return;
        }

        TagIO reader;
		reader.Open(music_file_path, TAG_IO_READ_MODE);
        auto cover = reader.embeddedCover();
        if (!cover.isNull()) {
            //cover = image_util::mergeImage({ cover });
            emit setAlbumCover(id.second.value(), qImageCache.addImage(cover));
            return;
        }

        // 4. If not found embedded cover, try to find cover from album folder.
        cover = qImageCache.scanCoverFromDir(QString::fromStdWString(music_file_path));
        if (!cover.isNull()) {
            //cover = image_util::mergeImage({ cover });
            emit setAlbumCover(id.second.value(), qImageCache.addImage(cover, true));
        }
	}
	catch (const std::exception &e) {
        XAMP_LOG_DEBUG("Find album cover error: {}", e.what());
	}    
}