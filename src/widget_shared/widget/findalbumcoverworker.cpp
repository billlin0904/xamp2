#include <widget/findalbumcoverworker.h>

#include <widget/database.h>
#include <widget/databasefacade.h>
#include <widget/imagecache.h>

FindAlbumCoverWorker::FindAlbumCoverWorker() {
    database_ptr_ = GetPooledDatabase();
}

void FindAlbumCoverWorker::OnCancelRequested() {
    is_stop_ = true;
}

void FindAlbumCoverWorker::OnFindAlbumCover(int32_t album_id,
    const QString& album,
    const std::wstring& file_path) {
    if (is_stop_) {
        return;
    }

    const auto cover_id = database_ptr_->Acquire()->GetAlbumCoverId(album_id);
    if (!cover_id.isEmpty()) {
        return;
    }

    std::wstring find_file_path;
    const auto first_file_path = database_ptr_->Acquire()->GetAlbumFirstMusicFilePath(album_id);
    if (!first_file_path) {
        find_file_path = file_path;
    }
    else {
        find_file_path = (*first_file_path).toStdWString();
    }

    const auto temp_path = Fs::temp_directory_path();
    if (file_path.find(temp_path.wstring()) != std::wstring::npos) {
        return;
    }

    const CoverArtReader reader;
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
    }
}