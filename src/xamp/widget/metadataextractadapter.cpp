#include <QMap>
#include <QDirIterator>

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

inline constexpr size_t kCachePreallocateSize = 500;

using xamp::metadata::TaglibMetadataReader;

static QList<QString> GetDirList(QString const& root_dir) {
    QList<QString> dir_or_files;
    QDirIterator itr(root_dir, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    while (itr.hasNext()) {
        dir_or_files.append(itr.next());
    }
    return dir_or_files;
}

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
    auto cover_id = Singleton<Database>::GetInstance().GetAlbumCoverId(album_id);
	
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
        Singleton<Database>::GetInstance().SetAlbumCover(album_id, album, cover_id);
    }	
    return cover_id;
}

std::tuple<int32_t, int32_t, QString> DatabaseIdCache::AddCache(const QString &album, const QString &artist) const {
    int32_t artist_id = 0;
    if (auto artist_id_op = this->artist_id_cache_.Find(artist)) {
        artist_id = *artist_id_op.value();
    }
    else {
        artist_id = Singleton<Database>::GetInstance().AddOrUpdateArtist(artist);
        this->artist_id_cache_.Insert(artist, artist_id);
    }

    int32_t album_id = 0;
    if (auto album_id_op = this->album_id_cache_.Find(album)) {
        album_id = *album_id_op.value();
    }
    else {
        album_id = Singleton<Database>::GetInstance().AddOrUpdateAlbum(album, artist_id);
        this->album_id_cache_.Insert(album, album_id);
    }

    QString cover_id;
    if (auto cover_id_op = this->cover_id_cache_.Find(album_id)) {
        cover_id = *cover_id_op.value();
    }

    return std::make_tuple(album_id, artist_id, cover_id);
}

using MetadataExtractAdapterBase = xamp::metadata::MetadataExtractAdapter;
using xamp::metadata::Metadata;
using xamp::metadata::Path;

struct ExtractAdapterProxy : public MetadataExtractAdapterBase {
    ExtractAdapterProxy(::MetadataExtractAdapter *adapter)
        : adapter_(adapter) {
        metadatas_.reserve(kCachePreallocateSize);
    }

    void OnWalkFirst() override {
    }

    void OnWalk(const Path& path, Metadata metadata) override {
        metadatas_.push_back(std::move(metadata));
    }

    void OnWalkNext() override {
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
        adapter_->readCompleted(metadatas_);
    }

    bool IsCancel() const noexcept override {
        return cancel_;
    }

    void Cancel() override {
        cancel_ = true;
    }

    void Reset() override {
    }

    std::atomic<bool> cancel_;
    ::MetadataExtractAdapter* adapter_;
    std::vector<Metadata> metadatas_;
};

MetadataExtractAdapter::MetadataExtractAdapter(QObject* parent)
    : QObject(parent) {    
}

MetadataExtractAdapter::~MetadataExtractAdapter() = default;

void MetadataExtractAdapter::ReadFileMetadata(MetadataExtractAdapter *adapter, QString const & file_name) {
    const auto dirs = GetDirList(file_name);

    for (const auto & file_paht : dirs) {
        ThreadPool::GetInstance().Run([adapter, file_paht]()
            {
                try {
                    ExtractAdapterProxy proxy(adapter);
                    const Path path(file_paht.toStdWString());
                    TaglibMetadataReader reader;
                    WalkPath(path, &proxy, &reader);
                }
                catch (const std::exception& e) {
                    XAMP_LOG_DEBUG("WalkPath has exception: {}", e.what());
                }
            });
    }
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

        auto music_id = Singleton<Database>::GetInstance().AddOrUpdateMusic(metadata, playlist_id);

        auto [album_id, artist_id, cover_id] = cache.AddCache(album, artist);

        // Find cover id from database.
        if (cover_id.isEmpty()) {
            cover_id = Singleton<Database>::GetInstance().GetAlbumCoverId(album_id);
        }

        // Database not exist find others.
        if (cover_id.isEmpty()) {
            cover_id = cache.AddCoverCache(album_id, album, metadata);
        }

        IgnoreSqlError(Singleton<Database>::GetInstance().AddOrUpdateAlbumMusic(album_id, artist_id, music_id))

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

