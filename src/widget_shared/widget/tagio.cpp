#include <widget/tagio.h>
#include <widget/util/image_util.h>

TrackInfo TagIO::getTrackInfo(const Path& path) {
    const auto reader = MakeMetadataReader();
	reader->Open(path);
    return reader->Extract();
}

void TagIO::Open(const Path& path, TagIOMode mode) {
    switch (mode) {
	case TAG_IO_READ_MODE:
		reader_->Open(path);
		break;
	case TAG_IO_WRITE_MODE:
		writer_->Open(path);
        break;
    }
}

TagIO::TagIO()
    : reader_(MakeMetadataReader())
    , writer_(MakeMetadataWriter()) {
}

void TagIO::writeArtist(const QString& artist) {
    writer_->WriteArtist(artist.toStdWString());
}

void TagIO::writeAlbum(const QString& album) {
    writer_->WriteAlbum(album.toStdWString());
}

void TagIO::writeTitle(const QString& title) {
    writer_->WriteTitle(title.toStdWString());
}

void TagIO::writeTrack(uint32_t track) {
    writer_->WriteTrack(track);
}

void TagIO::writeGenre(const QString& genre) {
    writer_->WriteGenre(genre.toStdWString());
}

void TagIO::writeComment(const QString& comment) {
    writer_->WriteComment(comment.toStdWString());
}

void TagIO::writeYear(uint32_t year) {
    writer_->WriteYear(year);
}

bool TagIO::embeddedCover(QPixmap& image, size_t& image_size) const {
    auto buffer = reader_->ReadEmbeddedCover();
    image_size = 0;
    if (buffer) {
        image.loadFromData(reinterpret_cast<uchar*>(buffer->data()), buffer->size());
        image_size = buffer->size();
        return true;
    }
    return false;
}

QPixmap TagIO::embeddedCover() const {
    QPixmap pixmap;
    auto buffer = reader_->ReadEmbeddedCover();
    if (buffer) {
        pixmap.loadFromData(reinterpret_cast<uchar*>(buffer->data()), buffer->size());
    }
    return pixmap;
}

void TagIO::removeEmbeddedCover() {
    writer_->RemoveEmbeddedCover();
}

bool TagIO::canWriteEmbeddedCover() const {
    return writer_->CanWriteEmbeddedCover();
}

void TagIO::writeEmbeddedCover(const QPixmap& image) {
    if (image.isNull()) {
        return;
    }

    const auto buffer = image_util::image2Buffer(image);
    writer_->WriteEmbeddedCover(buffer);
}
