#include <QApplication>
#include <QMap>
#include <QDebug>

#ifdef Q_OS_WIN
#include <execution>
#endif

#include "thememanager.h"
#include <widget/toast.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/pixmapcache.h>
#include <widget/image_utiltis.h>
#include <widget/metadataextractadapter.h>

using xamp::base::LruCache;

constexpr size_t kCachePreallocateSize = 100;

class IdCache {
public:
    static IdCache& instance() {
        static IdCache cache;
        return cache;
    }

    void clear() {
        album_id.Clear();
        artist_id.Clear();
        cover_id.Clear();
    }

    std::tuple<int32_t, int32_t, QString> getCache(const QString &album, const QString &artist);

    LruCache<int32_t, QString> cover_id;
private:
    IdCache() = default;

    LruCache<QString, int32_t> album_id;
    LruCache<QString, int32_t> artist_id;
};

std::tuple<int32_t, int32_t, QString> IdCache::getCache(const QString &album, const QString &artist) {
    int32_t artist_id = 0;
    if (auto artist_id_op = this->artist_id.Find(artist)) {
        artist_id = *artist_id_op.value();
    }
    else {
        artist_id = Database::instance().addOrUpdateArtist(artist);
        this->artist_id.Insert(artist, artist_id);
    }

    int32_t album_id = 0;
    if (auto album_id_op = this->album_id.Find(album)) {
        album_id = *album_id_op.value();
    }
    else {
        album_id = Database::instance().addOrUpdateAlbum(album, artist_id);
        this->album_id.Insert(album, album_id);
    }

    QString cover_id;
    if (auto cover_id_op = this->cover_id.Find(album_id)) {
        cover_id = *cover_id_op.value();
    }

    return std::make_tuple(album_id, artist_id, cover_id);
}

static void addImageCache(int32_t album_id, const QString &album, const Metadata &metadata) {
    using xamp::metadata::TaglibMetadataReader;

    auto cover_id = Database::instance().getAlbumCoverId(album_id);
    if (cover_id.isEmpty()) {
        TaglibMetadataReader cover_reader;
        QPixmap pixmap;
        const auto& buffer = cover_reader.ExtractEmbeddedCover(metadata.file_path);
        if (!buffer.empty()) {
            pixmap.loadFromData(buffer.data(),
                                static_cast<uint32_t>(buffer.size()));
        } else {
            pixmap = PixmapCache::instance().findExistCover(
                QString::fromStdWString(metadata.file_path));
        }
        if (!pixmap.isNull()) {
            cover_id = PixmapCache::instance().add(pixmap);
            assert(!cover_id.isEmpty());
            IdCache::instance().cover_id.Insert(album_id, cover_id);
            Database::instance().setAlbumCover(album_id, album, cover_id);
        }
    }
}

MetadataExtractAdapter::MetadataExtractAdapter(QObject* parent)
    : QObject(parent)
    , cancel_(false) {
    metadatas_.reserve(kCachePreallocateSize);
}

MetadataExtractAdapter::~MetadataExtractAdapter() {
    std::vector<Metadata>().swap(metadatas_);
}

void MetadataExtractAdapter::OnWalk(const Path&, Metadata metadata) {
    metadatas_.emplace_back(std::move(metadata));
}

void MetadataExtractAdapter::OnWalkNext() {
    if (metadatas_.empty()) {
        return;
    }
#ifdef Q_OS_WIN
    std::stable_sort(std::execution::par,
                     metadatas_.begin(), metadatas_.end(), [](const auto & first, const auto & sencond) {
        return first.track < sencond.track;
    });
#else
    std::stable_sort(
                metadatas_.begin(), metadatas_.end(), [](const auto & first, const auto & sencond) {
        return first.track < sencond.track;
    });
#endif
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
    auto playlist_id = -1;
    if (playlist != nullptr) {
        playlist_id = playlist->playlistId();
    }

    for (const auto& metadata : metadatas) {
        auto album = QString::fromStdWString(metadata.album);
        auto artist = QString::fromStdWString(metadata.artist);

        if (album.isEmpty()) {
            album = Q_UTF8(" ");
        }

        auto music_id = Database::instance().addOrUpdateMusic(metadata, playlist_id);

        auto [album_id, artist_id, cover_id] = IdCache::instance().getCache(album, artist);
        if (cover_id.isEmpty()) {
            addImageCache(album_id, album, metadata);
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
}
