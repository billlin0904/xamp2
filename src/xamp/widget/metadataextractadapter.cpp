#include <QApplication>
#include <QMap>
#include <QtConcurrent>
#include <QFuture>
#include <QMutexLocker>

#include "thememanager.h"
#include <widget/toast.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/image_utiltis.h>
#include <widget/metadataextractadapter.h>

inline constexpr size_t kCachePreallocateSize = 100;

using xamp::metadata::TaglibMetadataReader;

class DatabaseIdCache {
public:
    DatabaseIdCache() = default;

    std::tuple<int32_t, int32_t, QString> addCache(const QString& album, const QString& artist);

    QString addCoverCache(int32_t album_id, const QString& album, const Metadata& metadata, bool is_unknown_album);

    std::tuple<size_t, size_t, size_t> GetMissCount() const noexcept {
        return std::make_tuple(album_id_cache.GetMissCount(), artist_id_cache.GetMissCount(), cover_id_cache.GetMissCount());
    }
private:
    mutable LruCache<int32_t, QString> cover_id_cache;
    mutable LruCache<QString, int32_t> album_id_cache;
    mutable LruCache<QString, int32_t> artist_id_cache;
};

QString DatabaseIdCache::addCoverCache(int32_t album_id, const QString& album, const Metadata& metadata, bool is_unknown_album) {
    auto cover_id = Database::instance().getAlbumCoverId(album_id);
    if (cover_id.isEmpty()) {
        TaglibMetadataReader cover_reader;

        QPixmap pixmap;
        const auto& buffer = cover_reader.ExtractEmbeddedCover(metadata.file_path);
        if (!buffer.empty()) {
            pixmap.loadFromData(buffer.data(),
                static_cast<uint32_t>(buffer.size()));
        }
        else {
            if (!is_unknown_album) {
                pixmap = PixmapCache::instance().findFileDirCover(
                    QString::fromStdWString(metadata.file_path));
            }            
        }
        if (!pixmap.isNull()) {
            cover_id = PixmapCache::instance().add(pixmap);
            assert(!cover_id.isEmpty());
            cover_id_cache.Insert(album_id, cover_id);
            Database::instance().setAlbumCover(album_id, album, cover_id);
        }
    }
    return cover_id;
}

std::tuple<int32_t, int32_t, QString> DatabaseIdCache::addCache(const QString &album, const QString &artist) {
    int32_t artist_id = 0;
    if (auto artist_id_op = this->artist_id_cache.Find(artist)) {
        artist_id = *artist_id_op.value();
    }
    else {
        artist_id = Database::instance().addOrUpdateArtist(artist);
        this->artist_id_cache.Insert(artist, artist_id);
    }

    int32_t album_id = 0;
    if (auto album_id_op = this->album_id_cache.Find(album)) {
        album_id = *album_id_op.value();
    }
    else {
        album_id = Database::instance().addOrUpdateAlbum(album, artist_id);
        this->album_id_cache.Insert(album, album_id);
    }

    QString cover_id;
    if (auto cover_id_op = this->cover_id_cache.Find(album_id)) {
        cover_id = *cover_id_op.value();
    }

    return std::make_tuple(album_id, artist_id, cover_id);
}

MetadataExtractAdapter::MetadataExtractAdapter(QObject* parent)
    : QObject(parent)
    , cancel_(false) {
    metadatas_.reserve(kCachePreallocateSize);
}

MetadataExtractAdapter::~MetadataExtractAdapter() = default;

void MetadataExtractAdapter::readFileMetadata(MetadataExtractAdapter *adapter, QString const & file_name) {
    auto extract_handler = [adapter](const auto& file_name) {
        try {
        	const Path path(file_name.toStdWString());
        	TaglibMetadataReader reader;
            FromPath(path, adapter, &reader);
        }
        catch (const std::exception& e) {
            XAMP_LOG_DEBUG("FromPath has exception: {}", e.what());
        }
    };

    auto future = QtConcurrent::run(extract_handler, file_name);
    auto watcher = new QFutureWatcher<void>();
    (void)QObject::connect(watcher, &QFutureWatcher<void>::finished, [=]() {
        watcher->deleteLater();
        adapter->deleteLater();
        });

    watcher->setFuture(future);
}

void MetadataExtractAdapter::OnWalk(const Path&, Metadata metadata) {
    QMutexLocker locker{ &mutex_ };
    metadatas_.emplace_back(std::move(metadata));
}

void MetadataExtractAdapter::OnWalkNext() {
    QMutexLocker locker{ &mutex_ };
    if (metadatas_.empty()) {
        return;
    }
    std::stable_sort(
        metadatas_.begin(), metadatas_.end(), [](const auto& first, const auto& sencond) {
            return first.track < sencond.track;
        });
    emit readCompleted(metadatas_);
    metadatas_.clear();
}

bool MetadataExtractAdapter::IsCancel() const {
    return cancel_;
}

void MetadataExtractAdapter::Cancel() {
    cancel_ = true;
}

void MetadataExtractAdapter::Reset() {
    cancel_ = false;
}

void MetadataExtractAdapter::processMetadata(const std::vector<Metadata>& metadatas, PlayListTableView* playlist) {
    DatabaseIdCache cache;

    auto playlist_id = -1;
    if (playlist != nullptr) {
        playlist_id = playlist->playlistId();
    }

    for (const auto& metadata : metadatas) {
        auto album = QString::fromStdWString(metadata.album);
        auto artist = QString::fromStdWString(metadata.artist);

        auto is_unknown_album = false;
        if (album.isEmpty()) {
            album = tr("Unknown album");
            is_unknown_album = true;
        }

        auto music_id = Database::instance().addOrUpdateMusic(metadata, playlist_id);

        auto [album_id, artist_id, cover_id] = cache.addCache(album, artist);

        // Find cover id from database.
        if (cover_id.isEmpty()) {
            cover_id = Database::instance().getAlbumCoverId(album_id);
        }

        // Database not exist find others.
        if (cover_id.isEmpty()) {
            cover_id = cache.addCoverCache(album_id, album, metadata, is_unknown_album);
        }        

        IgnoreSqlError(Database::instance().addOrUpdateAlbumMusic(album_id, artist_id, music_id))

        if (playlist != nullptr) {
            auto entity = PlayListTableView::fromMetadata(metadata);
            entity.music_id = music_id;
            entity.album_id = album_id;
            entity.artist_id = artist_id;
            entity.cover_id = cover_id;
            playlist->appendItem(entity);
        }        
    }

    auto [album_miss, artist_miss, cover_miss] = cache.GetMissCount();
    XAMP_LOG_DEBUG("Metadata read total:{} miss {}%, {}%, {}% ",
        metadatas.size(),
        album_miss * 100 / metadatas.size(),
        artist_miss * 100 / metadatas.size(),
        cover_miss * 100 / metadatas.size());
}
