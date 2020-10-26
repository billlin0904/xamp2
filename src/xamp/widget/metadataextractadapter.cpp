#include <QApplication>
#include <QMap>
#include <QtConcurrent>
#include <QFuture>

#include <base/base.h>
#include <metadata/taglibmetareader.h>

#ifdef XAMP_OS_WIN
#include <execution>
#endif

#include "thememanager.h"
#include <widget/toast.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/image_utiltis.h>
#include <widget/metadataextractadapter.h>

inline constexpr size_t kCachePreallocateSize = 100;

using xamp::metadata::TaglibMetadataReader;

class DatabaseIdCache final {
public:
    DatabaseIdCache() = default;

    std::tuple<int32_t, int32_t, QString> AddCache(const QString& album, const QString& artist) const;

    QString AddCoverCache(int32_t album_id, const QString& album, const Metadata& metadata) const;

    std::tuple<size_t, size_t, size_t> GetMissCount() const noexcept {
        return std::make_tuple(album_id_cache_.GetMissCount(), 
            artist_id_cache_.GetMissCount(),
            cover_id_cache_.GetMissCount());
    }
private:	
    mutable LruCache<int32_t, QString> cover_id_cache_;
    mutable LruCache<QString, int32_t> album_id_cache_;
    mutable LruCache<QString, int32_t> artist_id_cache_;
};

QString DatabaseIdCache::AddCoverCache(int32_t album_id, const QString& album, const Metadata& metadata) const {
    auto cover_id = Database::instance().GetAlbumCoverId(album_id);
	
    if (!cover_id.isEmpty()) {
    	return cover_id;
    }
        
    TaglibMetadataReader cover_reader;

    QPixmap pixmap;
    const auto& buffer = cover_reader.ExtractEmbeddedCover(metadata.file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(),
            static_cast<uint32_t>(buffer.size()));
    }
    else {
         pixmap = Singleton<PixmapCache>::Get().FindFileDirCover(
                QString::fromStdWString(metadata.file_path));  
    }
    if (!pixmap.isNull()) {
        cover_id = Singleton<PixmapCache>::Get().Add(pixmap);
        assert(!cover_id.isEmpty());
        cover_id_cache_.Insert(album_id, cover_id);
        Database::instance().SetAlbumCover(album_id, album, cover_id);
    }	
    return cover_id;
}

std::tuple<int32_t, int32_t, QString> DatabaseIdCache::AddCache(const QString &album, const QString &artist) const {
    int32_t artist_id = 0;
    if (auto artist_id_op = this->artist_id_cache_.Find(artist)) {
        artist_id = *artist_id_op.value();
    }
    else {
        artist_id = Database::instance().AddOrUpdateArtist(artist);
        this->artist_id_cache_.Insert(artist, artist_id);
    }

    int32_t album_id = 0;
    if (auto album_id_op = this->album_id_cache_.Find(album)) {
        album_id = *album_id_op.value();
    }
    else {
        album_id = Database::instance().AddOrUpdateAlbum(album, artist_id);
        this->album_id_cache_.Insert(album, album_id);
    }

    QString cover_id;
    if (auto cover_id_op = this->cover_id_cache_.Find(album_id)) {
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

void MetadataExtractAdapter::ReadFileMetadata(MetadataExtractAdapter *adapter, QString const & file_name) {
    auto extract_handler = [adapter](const auto& file_name) {
        try {
        	const Path path(file_name.toStdWString());
        	TaglibMetadataReader reader;
            WalkPath(path, adapter, &reader);
        }
        catch (const std::exception& e) {
            XAMP_LOG_DEBUG("WalkPath has exception: {}", e.what());
        }
    };

    auto future = QtConcurrent::run(extract_handler, file_name);
    auto* watcher = new QFutureWatcher<void>();
    (void)QObject::connect(watcher, &QFutureWatcher<void>::finished, [=]() {
        watcher->deleteLater();
        adapter->deleteLater();
        });

    watcher->setFuture(future);
}

void MetadataExtractAdapter::OnWalkFirst() {
	watch_.Reset();
}

void MetadataExtractAdapter::OnWalk(const Path&, Metadata metadata) {
    metadatas_.push_back(std::move(metadata));
}

void MetadataExtractAdapter::OnWalkNext() {
    if (metadatas_.empty()) {
        return;
    }
    #ifdef XAMP_OS_WIN
    std::stable_sort(std::execution::par,
    #else
    std::stable_sort(
    #endif
        metadatas_.begin(), metadatas_.end(), [](const auto& first, const auto& last) {
            return first.track < last.track;
        });
    emit readCompleted(metadatas_);
    metadatas_.clear();
	XAMP_LOG_DEBUG("MetadataExtract elapsed {} sec", watch_.ElapsedSeconds());
}

bool MetadataExtractAdapter::IsCancel() const noexcept {
    return cancel_;
}

void MetadataExtractAdapter::Cancel() {
    cancel_ = true;
}

void MetadataExtractAdapter::Reset() {
    cancel_ = false;
}

void MetadataExtractAdapter::ProcessMetadata(const std::vector<Metadata>& result, PlayListTableView* playlist) {
    xamp::base::Stopwatch watch;
	watch.Reset();
	
    DatabaseIdCache cache;

    auto playlist_id = -1;
    if (playlist != nullptr) {
        playlist_id = playlist->playlistId();
    }

    // TODO: �d��artist���ɭԨϥγ̪����ת��W��.
    auto itr = std::max_element(result.begin(), result.end(), [](auto largest, auto next) {
        return next.artist.length() > largest.artist.length();
        });

    auto artist = QString::fromStdWString((*itr).artist);

    for (const auto& metadata : result) {
        auto album = QString::fromStdWString(metadata.album);

        auto is_unknown_album = false;
        if (album.isEmpty()) {
            album = tr("Unknown album");
            is_unknown_album = true;
        }

        auto music_id = Database::instance().AddOrUpdateMusic(metadata, playlist_id);

        auto [album_id, artist_id, cover_id] = cache.AddCache(album, artist);

        // Find cover id from database.
        if (cover_id.isEmpty()) {
            cover_id = Database::instance().GetAlbumCoverId(album_id);
        }

        // Database not exist find others.
        if (cover_id.isEmpty()) {
        	if (is_unknown_album) {
        		cover_id = Singleton<PixmapCache>::Get().GetUnknownCoverId();
        	} else {
        		cover_id = cache.AddCoverCache(album_id, album, metadata);
        	}            
        }        

        IgnoreSqlError(Database::instance().AddOrUpdateAlbumMusic(album_id, artist_id, music_id))

        if (playlist != nullptr) {
            auto entity = PlayListTableView::fromMetadata(metadata);
            entity.music_id = music_id;
            entity.album_id = album_id;
            entity.artist_id = artist_id;
            entity.cover_id = cover_id;
            playlist->appendItem(entity);
        }        
    }
	XAMP_LOG_DEBUG("ProcessMetadata elapsed {} sec", watch.ElapsedSeconds());
}
