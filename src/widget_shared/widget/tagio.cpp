#include <widget/tagio.h>
#include <widget/util/image_util.h>

TrackInfo TagIO::getTrackInfo(const Path& path) {
    const auto reader = MakeMetadataReader();
    return reader->Extract(path);
}

TagIO::TagIO()
    : reader_(MakeMetadataReader())
    , writer_(MakeMetadataWriter()) {
}

void TagIO::writeArtist(const Path& path, const QString& artist) {
    writer_->WriteArtist(path, artist.toStdWString());
}

void TagIO::writeAlbum(const Path& path, const QString& album) {
    writer_->WriteAlbum(path, album.toStdWString());
}

void TagIO::writeTitle(const Path& path, const QString& title) {
    writer_->WriteTitle(path, title.toStdWString());
}

void TagIO::writeTrack(const Path& path, uint32_t track) {
    writer_->WriteTrack(path, track);
}

void TagIO::writeGenre(const Path& path, const QString& genre) {
    writer_->WriteGenre(path, genre.toStdWString());
}

void TagIO::writeComment(const Path& path, const QString& comment) {
    writer_->WriteComment(path, comment.toStdWString());
}

void TagIO::writeYear(const Path& path, uint32_t year) {
    writer_->WriteYear(path, year);
}

bool TagIO::embeddedCover(const Path& file_path, QPixmap& image, size_t& image_size) const {
    auto buffer = reader_->ReadEmbeddedCover(file_path);
    image_size = 0;
    if (buffer) {
        image.loadFromData(reinterpret_cast<uchar*>(buffer->data()), buffer->size());
        image_size = buffer->size();
        return true;
    }
    return false;
}

QPixmap TagIO::embeddedCover(const Path& file_path) const {
    QPixmap pixmap;
    auto buffer = reader_->ReadEmbeddedCover(file_path);
    if (buffer) {
        pixmap.loadFromData(reinterpret_cast<uchar*>(buffer->data()), buffer->size());
    }
    return pixmap;
}

void TagIO::removeEmbeddedCover(const Path& file_path) {
    writer_->RemoveEmbeddedCover(file_path);
}

bool TagIO::canWriteEmbeddedCover(const Path& path) const {
    return writer_->CanWriteEmbeddedCover(path);
}

void TagIO::writeEmbeddedCover(const Path& file_path, const QPixmap& image) {
    if (image.isNull()) {
        return;
    }

    const auto buffer = image_util::image2Buffer(image);
    writer_->WriteEmbeddedCover(file_path, buffer);
}

QPixmap TagIO::embeddedCover(const TrackInfo& track_info) const {
    return embeddedCover(track_info.file_path);
}
