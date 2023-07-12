#include <widget/findalbumcoverworker.h>

#include <base/executor.h>

#include <widget/database.h>
#include <widget/databasefacade.h>
#include <widget/imagecache.h>

FindAlbumCoverWorker::FindAlbumCoverWorker() = default;

void FindAlbumCoverWorker::OnCancelRequested() {
    is_stop_ = true;
}

void FindAlbumCoverWorker::OnFindAlbumCover(int32_t album_id,
    const QString& album,
    const std::wstring& file_path) {
    if (is_stop_) {
        return;
    }

    /*const auto cover_id = qSharedDatabase.GetAlbumCoverId(album_id);
    if (!cover_id.isEmpty()) {
        return;
    }

    std::wstring find_file_path;
    const auto first_file_path = qDatabase.GetAlbumFirstMusicFilePath(album_id);
    if (!first_file_path) {
        find_file_path = file_path;
    }
    else {
        find_file_path = (*first_file_path).toStdWString();
    }

    CoverArtReader reader;
    auto cover = reader.GetEmbeddedCover(find_file_path);
    if (!cover.isNull()) {
        emit SetAlbumCover(album_id, album, qPixmapCache.AddImage(cover));
        return;
    }

    cover = ImageCache::ScanCoverFromDir(QString::fromStdWString(file_path));
    if (!cover.isNull()) {
        emit SetAlbumCover(album_id, album, qPixmapCache.AddImage(cover));
    }
    else {
        emit SetAlbumCover(album_id, album, qPixmapCache.GetUnknownCoverId());
    }*/
}