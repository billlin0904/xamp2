#include <QApplication>
#include <QMap>

#ifdef _WIN32
#include <execution>
#endif

#include "toast.h"
#include "database.h"
#include "playlisttableview.h"
#include "pixmapcache.h"
#include "image_utiltis.h"
#include "thememanager.h"
#include "metadataextractadapter.h"

constexpr size_t PREALLOCATE_SIZE = 100;

MetadataExtractAdapter::MetadataExtractAdapter(PlayListTableView* playlist)
    : QObject(nullptr)
    , cancel_(false)
    , playlist_(playlist) {
    metadatas_.reserve(PREALLOCATE_SIZE);
}

void MetadataExtractAdapter::OnWalk(const xamp::metadata::Path&, xamp::base::Metadata metadata) {
    metadatas_.push_back(metadata);
}

void MetadataExtractAdapter::OnWalkNext() {
#ifdef _WIN32
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
    processAndNotify(metadatas_);
    std::vector<xamp::base::Metadata>().swap(metadatas_);
    metadatas_.reserve(PREALLOCATE_SIZE);
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

void MetadataExtractAdapter::processAndNotify(const std::vector<xamp::base::Metadata>& metadatas) {
    QHash<QString, int32_t> album_id_cache;
    QHash<QString, int32_t> artist_id_cache;
    QHash<int32_t, QString> cover_id_cache;

    QString album;
    QString artist;

    auto playlist_id = playlist_->playlistId();
    if (!addPlayslist) {
        playlist_id = -1;
    }

    for (const auto & metadata : metadatas) {
        if (IsCancel()) {
            return;
        }

        album = QString::fromStdWString(metadata.album);
        artist = QString::fromStdWString(metadata.artist);
        
        auto music_id = Database::Instance().addOrUpdateMusic(metadata, playlist_id);

        int32_t artist_id = 0;
        auto artist_itr = artist_id_cache.find(artist);
        if (artist_itr != artist_id_cache.end()) {
            artist_id = (*artist_itr);
        } else {
            artist_id = Database::Instance().addOrUpdateArtist(artist);
            artist_id_cache[artist] = artist_id;
        }

        int32_t album_id = 0;
        auto album_itr = album_id_cache.find(album);
        if (album_itr != album_id_cache.end()) {
            album_id = (*album_itr);
        } else {
            album_id = Database::Instance().addOrUpdateAlbum(album, artist_id);
            album_id_cache[album] = album_id;
        }

        auto cover_id = Database::Instance().getAlbumCoverId(album_id);
        if (cover_id.isEmpty()) {
            auto cover_itr = cover_id_cache.find(album_id);
            if (cover_itr == cover_id_cache.end()) {
                QPixmap pixmap;
                const auto& buffer = cover_reader_.ExtractEmbeddedCover(metadata.file_path);
                if (!buffer.empty()) {
                    pixmap.loadFromData(buffer.data(), (uint32_t) buffer.size());
                } else {
                    pixmap = PixmapCache::findDirExistCover(QString::fromStdWString(metadata.file_path));
                }
                if (!pixmap.isNull()) {
                    cover_id = PixmapCache::Instance().emplace(std::move(pixmap));
                    cover_id_cache.insert(album_id, cover_id);
                    Database::Instance().setAlbumCover(album_id, album, cover_id);
                }                
            }
            else {
                cover_id = (*cover_itr);
            }
        }

        IgnoreSqlError(Database::Instance().addOrUpdateAlbumMusic(album_id, artist_id, music_id))

        if (addPlayslist) {
            auto entity = PlayListTableView::fromMetadata(metadata);
            entity.music_id = music_id;
            entity.album_id = album_id;
            entity.artist_id = artist_id;
            entity.cover_id = cover_id;
            playlist_->appendItem(entity);
        }

        qApp->processEvents();
    }
    emit finish();
}
