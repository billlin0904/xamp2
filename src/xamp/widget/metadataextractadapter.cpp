#include <atomic>
#include <utility>
#include <execution>
#include <forward_list>

#include <QMap>
#include <QDirIterator>
#include <QProgressDialog>

#include <base/base.h>
#include <base/str_utilts.h>
#include <base/threadpool.h>
#include <base/logger_impl.h>

#include <metadata/api.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadataextractadapter.h>

#include <player/audio_util.h>

#include "thememanager.h"

#include <widget/ui_utilts.h>
#include <widget/read_utiltis.h>
#include <widget/http.h>
#include <widget/toast.h>
#include <widget/database.h>
#include <widget/appsettings.h>
#include <widget/playlisttableview.h>
#include <widget/pixmapcache.h>
#include <widget/metadataextractadapter.h>

class DatabaseIdCache final {
public:
    DatabaseIdCache()
        : cover_reader_(MakeMetadataReader()) {
    }

    QPixmap getEmbeddedCover(const Metadata& metadata) const;

    std::tuple<int32_t, int32_t, QString> addOrGetAlbumAndArtistId(int64_t dir_last_write_time, const QString& album, const QString& artist, bool is_podcast) const;

    QString addCoverCache(int32_t album_id, const QString& album, const Metadata& metadata, bool is_unknown_album) const;
private:	
    mutable LruCache<int32_t, QString> cover_id_cache_;
    // Key: Album + Artist
    mutable LruCache<QString, int32_t> album_id_cache_;
    mutable LruCache<QString, int32_t> artist_id_cache_;
    AlignPtr<IMetadataReader> cover_reader_;
};

QPixmap DatabaseIdCache::getEmbeddedCover(const Metadata& metadata) const {
    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(metadata.file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(),
            static_cast<uint32_t>(buffer.size()));
    }
    return pixmap;
}

QString DatabaseIdCache::addCoverCache(int32_t album_id, const QString& album, const Metadata& metadata, bool is_unknown_album) const {
    auto cover_id = qDatabase.getAlbumCoverId(album_id);
    if (!cover_id.isEmpty()) {
    	return cover_id;
    }

    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(metadata.file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(), static_cast<uint32_t>(buffer.size()));
    }
    else {
    	if (!is_unknown_album) {
            pixmap = PixmapCache::findFileDirCover(QString::fromStdWString(metadata.file_path));
    	}          
    }

    if (!pixmap.isNull()) {
        cover_id = qPixmapCache.savePixamp(pixmap);
        XAMP_ASSERT(!cover_id.isEmpty());
        cover_id_cache_.AddOrUpdate(album_id, cover_id);
        qDatabase.setAlbumCover(album_id, album, cover_id);
    }	
    return cover_id;
}

std::tuple<int32_t, int32_t, QString> DatabaseIdCache::addOrGetAlbumAndArtistId(int64_t dir_last_write_time, const QString &album, const QString &artist, bool is_podcast) const {
    int32_t artist_id = 0;
    if (auto const * artist_id_op = this->artist_id_cache_.Find(artist)) {
        artist_id = *artist_id_op;
    }
    else {
        artist_id = qDatabase.addOrUpdateArtist(artist);
        this->artist_id_cache_.AddOrUpdate(artist, artist_id);
    }

    int32_t album_id = 0;
    if (auto const* album_id_op = this->album_id_cache_.Find(album + artist)) {
        album_id = *album_id_op;
    }
    else {
        album_id = qDatabase.addOrUpdateAlbum(album, artist_id, dir_last_write_time, is_podcast);
        this->album_id_cache_.AddOrUpdate(album, album_id);
    }

    QString cover_id;
    if (auto const* cover_id_op = this->cover_id_cache_.Find(album_id)) {
        cover_id = *cover_id_op;
    }

    return std::make_tuple(album_id, artist_id, cover_id);
}

class ExtractAdapterProxy final : public IMetadataExtractAdapter {
public:
    explicit ExtractAdapterProxy(const QSharedPointer<::MetadataExtractAdapter> &adapter)
        : adapter_(adapter) {
    }

    [[nodiscard]] bool IsAccept(Path const& path) const noexcept override {
        if (!path.has_extension()) {
            return false;
        }
        using namespace audio_util;
        const auto file_ext = String::ToLower(path.extension().string());
        auto const& support_file_set = GetSupportFileExtensions();
        return support_file_set.find(file_ext) != support_file_set.end();
    }

    XAMP_DISABLE_COPY(ExtractAdapterProxy)

    void OnWalkNew() override {
        qApp->processEvents();
    }

    void OnWalk(const Path&, Metadata metadata) override {
        metadatas_.push_front(metadata);
        qApp->processEvents();
    }

    void OnWalkEnd(DirectoryEntry const& dir_entry) override {
        if (metadatas_.empty()) {
            return;
        }
        metadatas_.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });
        emit adapter_->readCompleted(ToTime_t(dir_entry.last_write_time()), metadatas_);
        metadatas_.clear();
        qApp->processEvents();
    }
	
private:
    QSharedPointer<::MetadataExtractAdapter> adapter_;
    ForwardList<Metadata> metadatas_;
};

::MetadataExtractAdapter::MetadataExtractAdapter(QObject* parent)
    : QObject(parent) {    
}

void ::MetadataExtractAdapter::readFileMetadata(const QSharedPointer<MetadataExtractAdapter>& adapter, QString const & file_path, bool show_progress_dialog, bool is_recursive) {
	auto dialog = 
        makeProgressDialog(tr("Read file metadata"),
	    tr("Read progress dialog"), 
	    tr("Cancel"));

    if (show_progress_dialog) {
        dialog->show();
    }

    dialog->setMinimumDuration(1000);

    QList<QString> dirs;

    QDirIterator itr(file_path, is_recursive ? (QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot) : (QDir::Files));
    while (itr.hasNext()) {
        dirs.append(itr.next());
        qApp->processEvents();
    }

	if (dirs.isEmpty()) {
        dirs.push_back(file_path);
	}
    
    auto progress = 0;
    dialog->setMaximum(dirs.count());

    ExtractAdapterProxy proxy(adapter);

    const auto reader = MakeMetadataReader();

    for (const auto& file_dir_or_path : dirs) {
        if (dialog->wasCanceled()) {
            return;
        }

        dialog->setLabelText(file_dir_or_path);
        
        try {            
            const Path path = fromQStringPath(file_dir_or_path).toStdWString();
            
            if (!is_recursive) {
                ScanFolder(path, &proxy, reader.get());
            }
            else {
                RecursiveScanFolder(path, &proxy, reader.get());
            }            
        }
        catch (const std::exception& e) {
            XAMP_LOG_DEBUG("WalkPath has exception: {}", e.what());
        }
        dialog->setValue(progress++);
    }
}

void ::MetadataExtractAdapter::processMetadata(int64_t dir_last_write_time, const ForwardList<Metadata>& result, PlayListTableView* playlist, bool is_podcast) {
	auto playlist_id = -1;
    if (playlist != nullptr) {
        playlist_id = playlist->playlistId();
    }

	const DatabaseIdCache cache;

    for (const auto& metadata : result) {	    
	    auto album = QString::fromWCharArray(metadata.album.c_str());
        auto artist = QString::fromWCharArray(metadata.artist.c_str());
        
        auto is_unknown_album = false;
        if (album.isEmpty()) {
            album = tr("Unknown album");
            is_unknown_album = true;
            // todo: 如果有內建圖片就把當作一張專輯.
            auto cover = cache.getEmbeddedCover(metadata);
            if (!cover.isNull()) {
                album = QString::fromStdWString(metadata.file_name_no_ext);
            }
        }

    	if (artist.isEmpty()) {
            artist = tr("Unknown artist");
    	}

        const auto music_id = qDatabase.addOrUpdateMusic(metadata, playlist_id);

        auto [album_id, artist_id, cover_id] = cache.addOrGetAlbumAndArtistId(dir_last_write_time, album, artist, is_podcast);

        // Find cover id from database.
        if (cover_id.isEmpty()) {
            cover_id = qDatabase.getAlbumCoverId(album_id);
        }

        // Database not exist find others.
        if (cover_id.isEmpty()) {
            cover_id = cache.addCoverCache(album_id, album, metadata, is_unknown_album);
        }

        IgnoreSqlError(qDatabase.addOrUpdateAlbumMusic(album_id, artist_id, music_id))
    }

    if (playlist != nullptr) {
        playlist->updateData();
    }
}

Metadata getMetadata(QString const& file_path) {
    const Path path(file_path.toStdWString());
    auto reader = MakeMetadataReader();
    return reader->Extract(path);
}
