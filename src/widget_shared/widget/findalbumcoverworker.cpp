#include <QImageReader>

#include <base/object_pool.h>
#include <widget/findalbumcoverworker.h>
#include <widget/tagio.h>
#include <widget/database.h>
#include <widget/imagecache.h>
#include <widget/http.h>

FindAlbumCoverWorker::FindAlbumCoverWorker()
    : database_ptr_(GetPooledDatabase(2))
    , nam_(this) {
}

void FindAlbumCoverWorker::onFetchThumbnailUrl(const DatabaseCoverId& id, const QString& thumbnail_url) {
    auto download_handler = [id, this](const auto& content) {
        QPixmap image;
        if (!image.loadFromData(content)) {
            return;
        }
        emit setThumbnail(id, qImageCache.addImage(image));
        };

    auto error_handler = [id, thumbnail_url, this](const auto& url, const auto& error) {
        emit fetchThumbnailUrlError(id, thumbnail_url);
        };

    http::HttpClient(&nam_, thumbnail_url)
        .download(download_handler, error_handler);
}

void FindAlbumCoverWorker::cancelRequested() {
    is_stop_ = true;
}

void FindAlbumCoverWorker::onFindAlbumCover(int32_t album_id, const std::wstring& file_path) {
    if (is_stop_) {
        return;
    }

    const auto cover_id = database_ptr_->Acquire()->getAlbumCoverId(album_id);
    if (!cover_id.isEmpty()) {
        return;
    }

	std::wstring find_file_path;
    const auto first_file_path = database_ptr_->Acquire()->getAlbumFirstMusicFilePath(album_id);
    if (!first_file_path) {
        find_file_path = file_path;
    }
    else {
        find_file_path = first_file_path->toStdWString();
    }

    const auto temp_path = Fs::temp_directory_path();
    if (file_path.find(temp_path.wstring()) != std::wstring::npos) {
        return;
    }

    const TagIO reader;
    auto cover = reader.embeddedCover(find_file_path);
    if (!cover.isNull()) {
        emit setAlbumCover(album_id, qImageCache.addImage(cover));
        return;
    }

    cover = qImageCache.scanCoverFromDir(QString::fromStdWString(file_path));
    if (!cover.isNull()) {
        emit setAlbumCover(album_id, qImageCache.addImage(cover));
    }
    else {
        emit setAlbumCover(album_id, qImageCache.unknownCoverId());
    }
}