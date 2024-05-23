#include <QImageReader>
#include <widget/util/json_util.h>
#include <base/object_pool.h>

#include <widget/util/read_until.h>
#include <widget/worker/findalbumcoverworker.h>
#include <widget/tagio.h>
#include <widget/database.h>
#include <widget/imagecache.h>
#include <widget/http.h>

FindAlbumCoverWorker::FindAlbumCoverWorker()
    : database_ptr_(getPooledDatabase(2))
    , nam_(this)
    , buffer_pool_(MakeObjectPool<QByteArray>(kBufferPoolSize))
    , timer_(this) {
    (void)QObject::connect(&timer_, &QTimer::timeout, this, &FindAlbumCoverWorker::onLookupAlbumCoverTimeout);
    timer_.start(1000);
}

void FindAlbumCoverWorker::onFetchThumbnailUrl(const DatabaseCoverId& id, const QString& thumbnail_url) {
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

void FindAlbumCoverWorker::cancelRequested() {
    is_stop_ = true;
    timer_.stop();
    fetch_album_cover_queue_.clear();
}

void FindAlbumCoverWorker::onLookupAlbumCover(const DatabaseCoverId& id, const Path& path) {
    const auto entity = std::make_pair(id, path);
    fetch_album_cover_queue_.push_back(entity);
}

void FindAlbumCoverWorker::onLookupAlbumCoverTimeout() {
    if (is_stop_) {
        return;
    }

    if (fetch_album_cover_queue_.empty()) {
        return;
    }

    auto [id, path] = fetch_album_cover_queue_.front();
    fetch_album_cover_queue_.pop_front();
    lookupAlbumCover(id, path);
}

void FindAlbumCoverWorker::lookupAlbumCover(const DatabaseCoverId& id, const Path& file_path) {
    // 1. Read fingerprint from music file.
    auto [duration, result] = read_until::readFingerprint(file_path);

    // 2. Fingerprint to QString.
    QByteArray buffer(reinterpret_cast<const char*>(result.data()), result.size());
    auto fingerprint = QString::fromLatin1(buffer);

    auto error_handler = [this, id](const auto& url, const auto& error) {
        if (id.second) {
            emit setAlbumCover(id.second.value(), qImageCache.unknownCoverId());
        }
        };

    auto success_handler = [this, id](const auto& url, const auto& content) {
        QJsonDocument json;
        if (!json_util::deserialize(content, json)) {
            return;
        }

        auto root = json.object();
        auto results = root[qTEXT("results")].toArray();
        for (const auto& result : results) {
            auto recordings = result.toObject()[qTEXT("recordings")].toArray();
            for (const auto& recording : recordings) {
                auto release_groups = recording.toObject()[qTEXT("releasegroups")].toArray();
                for (const auto& release_group : release_groups) {
                    auto cover_art_archive = release_group.toObject()[qTEXT("cover-art-archive")].toObject();
                    if (cover_art_archive[qTEXT("front")].toBool()) {
                        auto cover_art_url = release_group.toObject()[qTEXT("cover-art-url")].toString();
                        onFetchThumbnailUrl(id, cover_art_url);
                        return;
                    }
                }
            }
        }

        if (id.second) {
            emit setAlbumCover(id.second.value(), qImageCache.unknownCoverId());
        }
        };

    constexpr auto kHost = qTEXT("api.acoustid.org");
    constexpr auto kAPIKey = qTEXT("J0OsCydP14");

    // 3. Perform HTTP GET request to AcoustID API.
    http::HttpClient(&nam_, buffer_pool_,
        qSTR("http://%1/v2/lookup?client=%2&meta=recordings+releasegroups+compress&duration=%3&fingerprint=%4")
        .arg(kHost)
        .arg(kAPIKey)
        .arg((int32_t)duration)
        .arg(fingerprint)
    )
    .success(success_handler)
    .error(error_handler)
    .get();
}

void FindAlbumCoverWorker::onFindAlbumCover(const DatabaseCoverId& id) {
    if (is_stop_) {
        return;
    }

    auto db = database_ptr_->Acquire();

    try {
        auto cover_id = db->getAlbumCoverId(id.second.value());

        if (!isNullOfEmpty(cover_id) && cover_id != qImageCache.unknownCoverId()) {
            return;
        }

        // 1. Read embedded cover in music file.
        auto music_file_path = db->getMusicFilePath(id.first).toStdWString();
        
        if (music_file_path.empty()) {
            // 2. Read embedded cover in album first music file.
            if (auto file_path = db->getAlbumFirstMusicFilePath(id.second.value())) {
                music_file_path = file_path->toStdWString();
            }            
        }

        // 3. Read file embedded cover.
        QPixmap cover;
        if (!music_file_path.empty()) {
            if (!IsFilePath(music_file_path)) {
                return;
            }

            const TagIO reader;
            cover = reader.embeddedCover(music_file_path);
            if (!cover.isNull()) {
                emit setAlbumCover(id.second.value(), qImageCache.addImage(cover));
                return;
            }

            // 4. If not found embedded cover, try to find cover from album folder.
            cover = qImageCache.scanCoverFromDir(QString::fromStdWString(music_file_path));
            if (!cover.isNull()) {
                emit setAlbumCover(id.second.value(), qImageCache.addImage(cover, true));
                return;
            }

            // 4. If not found cover from album folder, try to find cover from AcoustID API.
            onLookupAlbumCover(id, music_file_path);
        }
	}
	catch (const std::exception &e) {
        XAMP_LOG_DEBUG("Find album cover error: {}", e.what());
	}    
}