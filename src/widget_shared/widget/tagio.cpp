#include <widget/tagio.h>
#include <widget/image_utiltis.h>

TrackInfo TagIO::GetTrackInfo(const Path& path) {
    const auto reader = MakeMetadataReader();
    return reader->Extract(path);
}

TagIO::TagIO()
    : reader_(MakeMetadataReader())
    , writer_(MakeMetadataWriter()) {
}

void TagIO::WriteArtist(const Path& path, const QString& artist) {
    writer_->WriteArtist(path, artist.toStdWString());
}

void TagIO::WriteAlbum(const Path& path, const QString& album) {
    writer_->WriteAlbum(path, album.toStdWString());
}

void TagIO::WriteTitle(const Path& path, const QString& title) {
    writer_->WriteTitle(path, title.toStdWString());
}

void TagIO::WriteTrack(const Path& path, uint32_t track) {
    writer_->WriteTrack(path, track);
}

void TagIO::WriteGenre(const Path& path, const QString& genre) {
    writer_->WriteGenre(path, genre.toStdWString());
}

void TagIO::WriteComment(const Path& path, const QString& comment) {
    writer_->WriteComment(path, comment.toStdWString());
}

void TagIO::WriteYear(const Path& path, uint32_t year) {
    writer_->WriteYear(path, year);
}

bool TagIO::GetEmbeddedCover(const Path& file_path, QPixmap& image, size_t& image_size) const {
    const auto& buffer = reader_->ReadEmbeddedCover(file_path);
    image_size = 0;
    if (!buffer.empty()) {
        image.loadFromData(buffer.data(), buffer.size());
        image_size = buffer.size();
        return true;
    }
    return false;
}

QPixmap TagIO::GetEmbeddedCover(const Path& file_path) const {
    QPixmap pixmap;
    const auto& buffer = reader_->ReadEmbeddedCover(file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(), buffer.size());
    }
    return pixmap;
}

void TagIO::RemoveEmbeddedCover(const Path& file_path) {
    writer_->RemoveEmbeddedCover(file_path);
}

bool TagIO::CanWriteEmbeddedCover(const Path& path) const {
    return writer_->CanWriteEmbeddedCover(path);
}

void TagIO::WriteEmbeddedCover(const Path& file_path, const QPixmap& image) {
    if (image.isNull()) {
        return;
    }

    const auto buffer = image_utils::Image2Buffer(image);
    writer_->WriteEmbeddedCover(file_path, buffer);
}

QPixmap TagIO::GetEmbeddedCover(const TrackInfo& track_info) const {
    return GetEmbeddedCover(track_info.file_path);
}
