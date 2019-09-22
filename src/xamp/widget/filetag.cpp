#include <QCryptographicHash>
#include <QFileInfo>
#include <QDataStream>

#include "filetag.h"

const static int32_t CHUNK_SIZE = 1 << 22;

static QByteArray sha1(const QByteArray& data) {
    QCryptographicHash sha1_hash(QCryptographicHash::Sha1);
    sha1_hash.addData(data);
    return sha1_hash.result();
}

static QString urlSafeBase64Encode(const QByteArray& data) {
    return QString(data.toBase64(QByteArray::Base64UrlEncoding));
}

QString FileTag::getTagId(const QByteArray &buffer) {
    if (buffer.size() <= CHUNK_SIZE) {
        const auto sha1_data = sha1(buffer);
        auto hash_data = sha1_data;
        hash_data.prepend(0x16);
        return urlSafeBase64Encode(hash_data);
    } else {
        auto chunk_count = buffer.size() / CHUNK_SIZE;
        if (buffer.size() % CHUNK_SIZE != 0) {
            chunk_count += 1;
        }

        QDataStream reader(buffer);
        QByteArray sha1_all_data;

        for (auto i = 0; i < chunk_count; i++) {
            qint64 chunk_size = CHUNK_SIZE;
            if (i == chunk_count - 1) {
                chunk_size = buffer.size() - (chunk_count - 1) * CHUNK_SIZE;
            }
            QByteArray buf(chunk_size, 0);
            reader.readRawData(buf.data(), chunk_size);
            const auto sha1_data = sha1(buf);
            sha1_all_data.append(sha1_data);
        }

        auto hash_data = sha1(sha1_all_data);
        hash_data.prepend(0x96);
        return urlSafeBase64Encode(hash_data);
    }
}

QString FileTag::getTagId(const QString& file_name) {
    QString etag = "";
    QFileInfo fi(file_name);

    if (fi.exists() && fi.permission(QFile::ReadUser)) {
        QFile fh(file_name);

        const auto opened = fh.open(QIODevice::ReadOnly);

        if (opened) {
            const auto file_len = fh.size();
            if (file_len <= CHUNK_SIZE) {
                const auto file_data = fh.readAll();
                const auto sha1_data = sha1(file_data);

                auto hash_data = sha1_data;
                hash_data.prepend(0x16);
                etag = urlSafeBase64Encode(hash_data);
            }
            else {
                auto chunk_count = file_len / CHUNK_SIZE;
                if (file_len % CHUNK_SIZE != 0) {
                    chunk_count += 1;
                }

                QByteArray sha1_all_data;
                for (auto i = 0; i < chunk_count; i++) {
                    qint64 chunk_size = CHUNK_SIZE;
                    if (i == chunk_count - 1) {
                        chunk_size = file_len - (chunk_count - 1) * CHUNK_SIZE;
                    }
                    const auto buf = fh.read(chunk_size);
                    const auto sha1_data = sha1(buf);
                    sha1_all_data.append(sha1_data);
                }

                auto hash_data = sha1(sha1_all_data);
                hash_data.prepend(0x96);
                etag = urlSafeBase64Encode(hash_data);
            }
            fh.close();
        }
    }
    return etag;
}
