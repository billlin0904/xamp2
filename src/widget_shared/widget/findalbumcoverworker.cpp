#include <QImageReader>

#include <widget/findalbumcoverworker.h>
#include <widget/tagio.h>
#include <widget/database.h>
#include <widget/imagecache.h>
#include <widget/http.h>

FindAlbumCoverWorker::FindAlbumCoverWorker()
	: database_ptr_(GetPooledDatabase(1))
	, searcher_(this) {
    (void)QObject::connect(&searcher_, &DiscogsCoverSearcher::SearchFinished,
        [this](auto id, const auto& cover) {
        emit SetAlbumCover(id, qImageCache.AddImage(cover));
        });
}

void FindAlbumCoverWorker::OnCancelRequested() {
    is_stop_ = true;
}

void FindAlbumCoverWorker::OnFindAlbumCover(int32_t album_id,
    const QString& album,
    const QString& artist,
    const std::wstring& file_path) {
    if (is_stop_) {
        return;
    }

    const auto cover_id = database_ptr_->Acquire()->GetAlbumCoverId(album_id);
    if (!cover_id.isEmpty()) {
        return;
    }

    /*std::wstring find_file_path;
    const auto first_file_path = database_ptr_->Acquire()->GetAlbumFirstMusicFilePath(album_id);
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
    auto cover = reader.GetEmbeddedCover(find_file_path);
    if (!cover.isNull()) {
        emit SetAlbumCover(album_id, qImageCache.AddImage(cover));
        return;
    }

    cover = qImageCache.ScanCoverFromDir(QString::fromStdWString(file_path));
    if (!cover.isNull()) {
        emit SetAlbumCover(album_id, qImageCache.AddImage(cover));
    }
    else {
        searcher_.Search(artist, album, album_id);
        emit SetAlbumCover(album_id, qImageCache.GetUnknownCoverId());
    }*/
    searcher_.Search(artist, album, album_id);
}