#include <QImageReader>
#include <widget/util/json_util.h>
#include <base/object_pool.h>
#include <widget/util/image_util.h>
#include <widget/util/read_until.h>
#include <widget/databasefacade.h>
#include <widget/worker/albumcoverservice.h>
#include <widget/tagio.h>
#include <widget/dao/albumdao.h>
#include <widget/dao/musicdao.h>
#include <widget/imagecache.h>
#include <widget/http.h>

AlbumCoverService::AlbumCoverService()
    : database_ptr_(getPooledDatabase(2))
    , nam_(this)
    , timer_(this)
    , buffer_pool_(MakeObjectPool<QByteArray>(kBufferPoolSize)) {
}

void AlbumCoverService::cleaup() {
    database_ptr_.reset();
	buffer_pool_.reset();
}

void AlbumCoverService::onFetchThumbnailUrl(const DatabaseCoverId& id, const QString& thumbnail_url) {
    if (is_stop_) {
        return;
    }

    auto download_handler = [id, this](const auto& content) {
        QPixmap image;
        if (!image.loadFromData(content)) {
            return;
        }
        emit setThumbnail(id, qImageCache.addImage(image));
        };

    auto error_handler = [id, thumbnail_url, this](const auto& url, const auto& error) {
        XAMP_LOG_DEBUG("Download thumbnail error: {}", error.toStdString());
        emit fetchThumbnailUrlError(id, thumbnail_url);
        };

    // Reduce memory use age by using ObjectPool and share QNetworkAccessManager.
    http::HttpClient(&nam_, buffer_pool_, thumbnail_url)
        .download(download_handler, error_handler);
}

void AlbumCoverService::cancelRequested() {
    is_stop_ = true;
    timer_.stop();
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
        try {
            auto image = tag_io.embeddedCover(entity.file_path.toStdWString());
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

    auto db = database_ptr_->Acquire();

    try {
        auto cover_id = dao::AlbumDao(db->getDatabase()).getAlbumCoverId(id.second.value());

        if (!isNullOfEmpty(cover_id) && cover_id != qImageCache.unknownCoverId()) {
            return;
        }

        // 1. Read embedded cover in music file.
        auto music_file_path = dao::MusicDao(db->getDatabase()).getMusicFilePath(id.first).toStdWString();
        
        if (music_file_path.empty()) {
            // 2. Read embedded cover in album first music file.
            if (auto file_path = dao::AlbumDao(db->getDatabase()).getAlbumFirstMusicFilePath(id.second.value())) {
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

        const TagIO reader;
        auto cover = reader.embeddedCover(music_file_path);
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