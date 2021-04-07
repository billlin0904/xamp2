#include <QMap>
#include <QDirIterator>
#include <QProgressDialog>

#include <base/base.h>
#include <base/str_utilts.h>
#include <base/threadpool.h>
#include <base/threadpool.h>
#include <metadata/taglibmetareader.h>

#include <player/audio_util.h>

#include <atomic>
#include <utility>
#include <metadata/metadatareader.h>
#include <widget/widget_shared.h>

#include "thememanager.h"

#include <widget/http.h>
#include <widget/toast.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/image_utiltis.h>
#include <widget/metadataextractadapter.h>

inline constexpr size_t kCachePreallocateSize = 500;

using xamp::metadata::TaglibMetadataReader;

class DatabaseIdCache final {
public:
    std::tuple<int32_t, int32_t, QString> AddCache(const QString& album, const QString& artist) const;

    QString AddCoverCache(int32_t album_id, const QString& album, const Metadata& metadata, bool is_unknown_album) const;
private:	
    mutable LruCache<int32_t, QString> cover_id_cache_;
    mutable LruCache<QString, int32_t> album_id_cache_;
    mutable LruCache<QString, int32_t> artist_id_cache_;
};

QString DatabaseIdCache::AddCoverCache(int32_t album_id, const QString& album, const Metadata& metadata, bool is_unknown_album) const {
    auto cover_id = Singleton<Database>::GetInstance().getAlbumCoverId(album_id);
	
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
    	if (!is_unknown_album) {
            pixmap = Singleton<PixmapCache>::GetInstance().findFileDirCover(
                QString::fromStdWString(metadata.file_path));
    	}          
    }
    if (!pixmap.isNull()) {
        cover_id = Singleton<PixmapCache>::GetInstance().addOrUpdate(pixmap);
        assert(!cover_id.isEmpty());
        cover_id_cache_.AddOrUpdate(album_id, cover_id);
        Singleton<Database>::GetInstance().setAlbumCover(album_id, album, cover_id);
    }	
    return cover_id;
}

std::tuple<int32_t, int32_t, QString> DatabaseIdCache::AddCache(const QString &album, const QString &artist) const {
    int32_t artist_id = 0;
    if (auto const * artist_id_op = this->artist_id_cache_.Find(artist)) {
        artist_id = *artist_id_op;
    }
    else {
        artist_id = Singleton<Database>::GetInstance().addOrUpdateArtist(artist);
        this->artist_id_cache_.AddOrUpdate(artist, artist_id);
    }

    int32_t album_id = 0;
    if (auto const* album_id_op = this->album_id_cache_.Find(album)) {
        album_id = *album_id_op;
    }
    else {
        album_id = Singleton<Database>::GetInstance().addOrUpdateAlbum(album, artist_id);
        this->album_id_cache_.AddOrUpdate(album, album_id);
    }

    QString cover_id;
    if (auto const* cover_id_op = this->cover_id_cache_.Find(album_id)) {
        cover_id = *cover_id_op;
    }

    return std::make_tuple(album_id, artist_id, cover_id);
}

using xamp::metadata::Metadata;
using xamp::metadata::Path;

class ExtractAdapterProxy : public xamp::metadata::MetadataExtractAdapter {
public:
    explicit ExtractAdapterProxy(const QSharedPointer<::MetadataExtractAdapter> &adapter)
        : adapter_(adapter) {
      metadatas_.reserve(kCachePreallocateSize);
    }

    [[nodiscard]] bool IsSupported(Path const& path) const noexcept override {
        using namespace xamp::player::audio_util;
        const auto file_ext = String::ToLower(path.extension().string());
        return audio_util::GetSupportFileExtensions().find(file_ext) !=
               audio_util::GetSupportFileExtensions().end();
    }

    XAMP_DISABLE_COPY(ExtractAdapterProxy)

    void OnWalkFirst() override {
        qApp->processEvents();
    }

    void OnWalk(const Path&, Metadata metadata) override {
        metadatas_.push_back(std::move(metadata));
        qApp->processEvents();
    }

    void OnWalkNext() override {
        if (metadatas_.empty()) {
            return;
        }
        std::stable_sort(
            metadatas_.begin(), metadatas_.end(), [](const auto& first, const auto& last) {
                return first.track < last.track;
            });
        adapter_->readCompleted(metadatas_);
        metadatas_.clear();
        qApp->processEvents();
    }
	
private:
    QSharedPointer<::MetadataExtractAdapter> adapter_;
    std::vector<Metadata> metadatas_;
};

MetadataExtractAdapter::MetadataExtractAdapter(QObject* parent)
    : QObject(parent) {    
}

MetadataExtractAdapter::~MetadataExtractAdapter() = default;

void MetadataExtractAdapter::readFileMetadata(const QSharedPointer<MetadataExtractAdapter>& adapter, QString const & file_path) {
    QProgressDialog dialog(tr("Read file metadata"), tr("Cancel"), 0, 0);
    dialog.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle(tr("Read progress dialog"));
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setMinimumSize(QSize(500, 100));
    dialog.setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
    dialog.show();
    dialog.setMinimumDuration(1000);

    QList<QString> dirs;
    QDirIterator itr(file_path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    while (itr.hasNext()) {
        dirs.append(itr.next());
        qApp->processEvents();
    }

	if (dirs.isEmpty()) {
        dirs.push_back(file_path);
	}
    
    auto progress = 0;
    dialog.setMaximum(dirs.count());

    ExtractAdapterProxy proxy(adapter);
	
    for (const auto& file_dir_or_path : dirs) {
    	if (dialog.wasCanceled()) {
            return;
    	}

        dialog.setLabelText(file_dir_or_path);
    	
        try {            
            const Path path(file_dir_or_path.toStdWString());
            TaglibMetadataReader reader;
            WalkPath(path, &proxy, &reader);            
        }
        catch (const std::exception& e) {
            XAMP_LOG_DEBUG("WalkPath has exception: {}", e.what());
        }

        dialog.setValue(progress++);
    }
}

void MetadataExtractAdapter::processMetadata(const std::vector<Metadata>& result, PlayListTableView* playlist) {
  const DatabaseIdCache cache;

    auto playlist_id = -1;
    if (playlist != nullptr) {
        playlist_id = playlist->playlistId();
    }

    for (const auto& metadata : result) {
        auto album = QString::fromStdWString(metadata.album);
        auto artist = QString::fromStdWString(metadata.artist);

        auto is_unknown_album = false;
        if (album.isEmpty()) {
            album = tr("Unknown album");
            is_unknown_album = true;
        }

    	if (artist.isEmpty()) {
            artist = tr("Unknown artist");
    	}

        const auto music_id = Singleton<Database>::GetInstance().addOrUpdateMusic(metadata, playlist_id);

        auto [album_id, artist_id, cover_id] = cache.AddCache(album, artist);

        // Find cover id from database.
        if (cover_id.isEmpty()) {
            cover_id = Singleton<Database>::GetInstance().getAlbumCoverId(album_id);
        }

        // Database not exist find others.
        if (cover_id.isEmpty()) {
            cover_id = cache.AddCoverCache(album_id, album, metadata, is_unknown_album);
        }

        IgnoreSqlError(Singleton<Database>::GetInstance().addOrUpdateAlbumMusic(album_id, artist_id, music_id))
    }

    if (playlist != nullptr) {
        playlist->refresh();
    }
}

