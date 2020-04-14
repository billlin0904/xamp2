#include <QApplication>
#include <QMap>
#include <QDebug>

#ifdef _WIN32
#include <execution>
#endif

#include "thememanager.h"
#include <widget/toast.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/pixmapcache.h>
#include <widget/image_utiltis.h>
#include <widget/metadataextractadapter.h>

constexpr size_t PREALLOCATE_SIZE = 100;

class MetadataExtractCache {
public:
    static MetadataExtractCache& instance() {
        static MetadataExtractCache cache;
        return cache;
    }

    void clear() {
        album_id.Clear();
        artist_id.Clear();
        cover_id.Clear();
    }

    xamp::base::LruCache<QString, int32_t> album_id;
    xamp::base::LruCache<QString, int32_t> artist_id;
    xamp::base::LruCache<int32_t, QString> cover_id;
private:
    MetadataExtractCache() = default;    
};

MetadataExtractAdapter::MetadataExtractAdapter(QObject* parent)
    : QObject(parent)
    , cancel_(false) {
    metadatas_.reserve(PREALLOCATE_SIZE);
}

MetadataExtractAdapter::~MetadataExtractAdapter() {
    std::vector<xamp::base::Metadata>().swap(metadatas_);
    MetadataExtractCache::instance().clear();
}

void MetadataExtractAdapter::OnWalk(const xamp::metadata::Path&, xamp::base::Metadata metadata) {
    metadatas_.push_back(metadata);
}

void MetadataExtractAdapter::OnWalkNext() {
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

void MetadataExtractAdapter::processMetadata(const std::vector<xamp::base::Metadata>& metadatas, PlayListTableView* playlist) {
    xamp::metadata::TaglibMetadataReader cover_reader;

    QString album;
    QString artist;

    auto playlist_id = -1;
    if (playlist != nullptr) {
        playlist_id = playlist->playlistId();
    }

    auto& cache = MetadataExtractCache::instance();

    for (const auto& metadata : metadatas) {
        album = QString::fromStdWString(metadata.album);
        artist = QString::fromStdWString(metadata.artist);

        if (album.isEmpty()) {
            album = Q_UTF8("?");
        }

        // TODO: 找尋相同的album和artist的作法不是很好?
        auto music_id = Database::instance().addOrUpdateMusic(metadata, playlist_id);

        int32_t artist_id = 0;
        if (auto artist_id_op = cache.artist_id.Find(artist)) {
            artist_id = *artist_id_op.value();
        }
        else {
            artist_id = Database::instance().addOrUpdateArtist(artist);
            cache.artist_id.Insert(artist, artist_id);
        }

        int32_t album_id = 0;
        if (auto album_id_op = cache.album_id.Find(album)) {
            album_id = *album_id_op.value();
        }
        else {
            album_id = Database::instance().addOrUpdateAlbum(album, artist_id);
            cache.album_id.Insert(album, album_id);
        }

        QString cover_id;        
        if (auto cover_id_op = cache.cover_id.Find(album_id)) {
            cover_id = *cover_id_op.value();
        }
        else {
            cover_id = Database::instance().getAlbumCoverId(album_id);
            if (cover_id.isEmpty()) {
                QPixmap pixmap;
                const auto& buffer = cover_reader.ExtractEmbeddedCover(metadata.file_path);
                if (!buffer.empty()) {
                    pixmap.loadFromData(buffer.data(), (uint32_t)buffer.size());
                }
                else {
                    pixmap = PixmapCache::findDirExistCover(QString::fromStdWString(metadata.file_path));
                }
                if (!pixmap.isNull()) {
                    cover_id = PixmapCache::instance().add(pixmap);
                    assert(!cover_id.isEmpty());
                    cache.cover_id.Insert(album_id, cover_id);
                    Database::instance().setAlbumCover(album_id, album, cover_id);
                }
            }
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
