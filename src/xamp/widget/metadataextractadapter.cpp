#include <QApplication>
#include <QMap>

#include <execution>
#include <metadata/taglibmetareader.h>

#include "toast.h"
#include "database.h"
#include "playlisttableview.h"
#include "pixmapcache.h"
#include "pixmapcache.h"
#include "metadataextractadapter.h"

const size_t MetadataExtractAdapter::PREALLOCATE_SIZE = 100;

MetadataExtractAdapter::MetadataExtractAdapter()
    : QObject(nullptr)
	, playlist(nullptr)
    , cancel_(false) {
	metadatas_.reserve(PREALLOCATE_SIZE);
}

void MetadataExtractAdapter::OnWalk(const xamp::metadata::Path& path, const xamp::base::Metadata& metadata) {    
    metadatas_.push_back(metadata);
}

void MetadataExtractAdapter::OnWalkNext() {    
	std::stable_sort(std::execution::par,
		metadatas_.begin(), metadatas_.end(), [](const auto & first, const auto & sencond) {
		return first.track < sencond.track;		
		});
	onCompleted(metadatas_);
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

void MetadataExtractAdapter::onCompleted(const std::vector<xamp::base::Metadata>& metadatas) {
	QHash<QString, int32_t> album_id_cache;
	QHash<QString, int32_t> artist_id_cache;
	QHash<int32_t, QString> cover_id_cache;

	xamp::metadata::TaglibMetadataReader cover_reader;

    QString album;
    QString artist;

	for (const auto & metadata : metadatas) {
		if (IsCancel()) {
			return;
		}

		album = QString::fromStdWString(metadata.album);
		artist = QString::fromStdWString(metadata.artist);

        auto music_id = Database::Instance().addOrUpdateMusic(metadata, playlist->playlistId());

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
				const auto& buffer = cover_reader.ExtractEmbeddedCover(metadata.file_path);
				if (!buffer.empty()) {					
					pixmap.loadFromData(buffer.data(), buffer.size());
				} else {
					pixmap = PixmapCache::findDirExistCover(QString::fromStdWString(metadata.file_path));
				}
				cover_id = PixmapCache::Instance().emplace(std::move(pixmap));
				cover_id_cache.insert(album_id, cover_id);
				Database::Instance().updateAlbumCover(album_id, album, cover_id);
			}
			else {
				cover_id = (*cover_itr);
			}
		}

		IF_FAILED_LOG(Database::Instance().addOrUpdateAlbumMusic(album_id, artist_id, music_id));

		auto entity = PlayListTableView::fromMetadata(metadata);
		entity.music_id = music_id;
		entity.album_id = album_id;
		entity.artist_id = artist_id;
		entity.cover_id = cover_id;

		playlist->appendItem(entity);
		qApp->processEvents();
	}	
	emit finish();
}
