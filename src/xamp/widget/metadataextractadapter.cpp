#include <QMap>
#include <QTemporaryFile>

#include <base/base.h>
#include <base/threadpool.h>
#include <base/threadpool.h>
#include <metadata/taglibmetareader.h>

#ifdef XAMP_OS_WIN
#include <execution>
#endif

#include "thememanager.h"

#include <widget/http.h>
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
         pixmap = Singleton<PixmapCache>::GetInstance().FindFileDirCover(
                QString::fromStdWString(metadata.file_path));  
    }
    if (!pixmap.isNull()) {
        cover_id = Singleton<PixmapCache>::GetInstance().Add(pixmap);
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
    ThreadPool::GetInstance().Run([adapter, file_name]()
        {
            try {
                const Path path(file_name.toStdWString());
                TaglibMetadataReader reader;
                WalkPath(path, adapter, &reader);
            }
            catch (const std::exception& e) {
                XAMP_LOG_DEBUG("WalkPath has exception: {}", e.what());
            }
        });
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
    DatabaseIdCache cache;

    auto playlist_id = -1;
    if (playlist != nullptr) {
        playlist_id = playlist->playlistId();
    }

    // TODO: 查找artist的時候使用最長長度的名稱.
    for (const auto& metadata : result) {
        auto album = QString::fromStdWString(metadata.album);
        auto artist = QString::fromStdWString(metadata.artist);

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
        		cover_id = Singleton<PixmapCache>::GetInstance().GetUnknownCoverId();
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

            const QUrl url_parse(entity.file_path);
            if (url_parse.scheme() == Q_UTF8("http") || url_parse.scheme() == Q_UTF8("https")) {
                entity.url = entity.file_path;
            }
        }
    }
}

void MetadataExtractAdapter::DownloadFile(const PlayListEntity &entity) {
    QSharedPointer<QTemporaryFile> tempfile(new QTemporaryFile());
    if (!tempfile->open()) {
        return;
    }

    http::HttpClient(entity.url.toString()).download([tempfile](const auto &buffer) {
        tempfile->write(buffer);
        tempfile->close();
    });
}
