#include <widget/util/tag_util.h>

#include <widget/util/image_util.h>

namespace tag_util {

std::optional<QImage> readEmbeddedCoverImage(IMetadataReader& reader) {
	const auto buffer = reader.readEmbeddedCover();
	if (!buffer) {
		return std::nullopt;
	}

	const auto& data = buffer.value();
	if (data.size() > static_cast<size_t>((std::numeric_limits<int>::max)())) {
		return std::nullopt;
	}

	QImage image;
	if (!image.loadFromData(reinterpret_cast<const uchar*>(data.data()), static_cast<int>(data.size()))) {
		return std::nullopt;
	}
	return image;
}

bool readEmbeddedCover(IMetadataReader& reader, QPixmap& image, size_t& image_size) {
    const auto buffer = reader.readEmbeddedCover();
    image_size = 0;
    if (!buffer) {
        return false;
    }

    const auto& data = buffer.value();
    if (!image.loadFromData(reinterpret_cast<const uchar*>(data.data()), data.size())) {
        return false;
    }
    image_size = data.size();
    return true;
}

QPixmap readEmbeddedCover(IMetadataReader& reader) {
    QPixmap image;
    size_t image_size = 0;
    readEmbeddedCover(reader, image, image_size);
    return image;
}

void writeEmbeddedCover(IMetadataWriter& writer, const QPixmap& image) {
    if (image.isNull()) {
        return;
    }

    writer.writeEmbeddedCover(image_util::image2JpegBuffer(image));
}

}
