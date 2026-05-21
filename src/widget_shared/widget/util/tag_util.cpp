#include <widget/util/tag_util.h>

#include <widget/util/image_util.h>

namespace tag_util {

bool readEmbeddedCover(xamp::metadata::IMetadataReader& reader, QPixmap& image, size_t& image_size) {
    const auto buffer = reader.ReadEmbeddedCover();
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

QPixmap readEmbeddedCover(xamp::metadata::IMetadataReader& reader) {
    QPixmap image;
    size_t image_size = 0;
    readEmbeddedCover(reader, image, image_size);
    return image;
}

void writeEmbeddedCover(xamp::metadata::IMetadataWriter& writer, const QPixmap& image) {
    if (image.isNull()) {
        return;
    }

    writer.WriteEmbeddedCover(image_util::image2Buffer(image));
}

}
