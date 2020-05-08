#include <QCryptographicHash>
#include <QFileInfo>
#include <QDataStream>

#include <widget/filetag.h>

constexpr int32_t kTagIdChunkSize = 1 << 22;
constexpr char kPrepareHeader = static_cast<char>(0x96);

static QByteArray sha1(const QByteArray& data) noexcept {
    QCryptographicHash sha1_hash(QCryptographicHash::Sha1);
    sha1_hash.addData(data);
    return sha1_hash.result();
}

static QString urlSafeBase64Encode(const QByteArray& data) noexcept {
    return QLatin1String(data.toBase64(QByteArray::Base64UrlEncoding));
}

QString FileTag::getTagId(const QByteArray &buffer) noexcept {
    if (buffer.size() <= kTagIdChunkSize) {
        const auto sha1_data = sha1(buffer);
        auto hash_data = sha1_data;
        hash_data.prepend(0x16);
        return urlSafeBase64Encode(hash_data);
    } else {
        auto chunk_count = static_cast<int32_t>(buffer.size())
                             / kTagIdChunkSize;
        if (buffer.size() % kTagIdChunkSize != 0) {
            chunk_count += 1;
        }

        QDataStream reader(buffer);
        QByteArray sha1_all_data;

        for (auto i = 0; i < chunk_count; i++) {
            auto chunk_size = kTagIdChunkSize;
            if (i == chunk_count - 1) {
                chunk_size = buffer.size() - (chunk_count - 1) * kTagIdChunkSize;
            }
            QByteArray buf(chunk_size, 0);
            reader.readRawData(buf.data(), chunk_size);
            const auto sha1_data = sha1(buf);
            sha1_all_data.append(sha1_data);
        }

        auto hash_data = sha1(sha1_all_data);
        hash_data.prepend(kPrepareHeader);
        return urlSafeBase64Encode(hash_data);
    }
}

QString FileTag::getTagId(const QString& file_name) noexcept {
	QString etag;
    QFileInfo fi(file_name);

    if (fi.exists() && fi.permission(QFile::ReadUser)) {
        QFile fh(file_name);

        const auto opened = fh.open(QIODevice::ReadOnly);

        if (opened) {
            const auto file_len = fh.size();
            if (file_len <= kTagIdChunkSize) {
                const auto file_data = fh.readAll();
                const auto sha1_data = sha1(file_data);

                auto hash_data = sha1_data;
                hash_data.prepend(0x16);
                etag = urlSafeBase64Encode(hash_data);
            }
            else {
                auto chunk_count = file_len / kTagIdChunkSize;
                if (file_len % kTagIdChunkSize != 0) {
                    chunk_count += 1;
                }

                QByteArray sha1_all_data;
                for (auto i = 0; i < chunk_count; i++) {
                    qint64 chunk_size = kTagIdChunkSize;
                    if (i == chunk_count - 1) {
                        chunk_size = file_len - (chunk_count - 1) * kTagIdChunkSize;
                    }
                    const auto buf = fh.read(chunk_size);
                    const auto sha1_data = sha1(buf);
                    sha1_all_data.append(sha1_data);
                }

                auto hash_data = sha1(sha1_all_data);
                hash_data.prepend(kPrepareHeader);
                etag = urlSafeBase64Encode(hash_data);
            }
            fh.close();
        }
    }
    return etag;
}
