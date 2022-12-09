#include <atomic>
#include <utility>
#include <execution>
#include <forward_list>
#include <unordered_set>

#include <QFile>
#include <QDirIterator>
#include <QMap>
#include <QProgressDialog>

#include <base/base.h>
#include <base/str_utilts.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/siphash.h>

#include <metadata/api.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadataextractadapter.h>

#include <player/audio_util.h>

#include "thememanager.h"

#include <widget/widget_shared.h>
#include <widget/ui_utilts.h>
#include <widget/read_utiltis.h>
#include <widget/http.h>
#include <widget/toast.h>
#include <widget/database.h>
#include <widget/appsettings.h>
#include <widget/playlisttableview.h>
#include <widget/pixmapcache.h>
#include <widget/metadataextractadapter.h>

DatabaseIdCache::DatabaseIdCache()
    : cover_reader_(MakeMetadataReader()) {
}

void DatabaseIdCache::clear() {
    cover_id_cache_.Clear();
    album_id_cache_.Clear();
    artist_id_cache_.Clear();
}

QPixmap DatabaseIdCache::getEmbeddedCover(const TrackInfo& metadata) const {
    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(metadata.file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(),
            static_cast<uint32_t>(buffer.size()));
    }
    return pixmap;
}

QString DatabaseIdCache::addCoverCache(int32_t album_id, const QString& album, const TrackInfo& metadata, bool is_unknown_album) const {
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

std::tuple<int32_t, int32_t, QString> DatabaseIdCache::addOrGetAlbumAndArtistId(int64_t dir_last_write_time,
    const QString &album,
    const QString &artist, 
    bool is_podcast,
    const QString& disc_id) const {
#if 1
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
        album_id = qDatabase.addOrUpdateAlbum(album, artist_id, dir_last_write_time, is_podcast, disc_id);
        this->album_id_cache_.AddOrUpdate(album + artist, album_id);
    }

    QString cover_id;
    if (auto const* cover_id_op = this->cover_id_cache_.Find(album_id)) {
        cover_id = *cover_id_op;
    }
#else
    auto artist_id = qDatabase.addOrUpdateArtist(artist);
    auto album_id = qDatabase.addOrUpdateAlbum(album, artist_id, dir_last_write_time, is_podcast, disc_id);
    QString cover_id;
#endif
    return std::make_tuple(album_id, artist_id, cover_id);
}

class TrackInfoExtractAdapterProxy final : public IMetadataExtractAdapter {
public:
    explicit TrackInfoExtractAdapterProxy(const QSharedPointer<::MetadataExtractAdapter> &adapter)
        : reader_(MakeMetadataReader())
		, adapter_(adapter) {
        GetSupportFileExtensions();
    }

    [[nodiscard]] bool IsAccept(Path const& path) const noexcept override {
        if (Fs::is_directory(path)) {
            return true;
        }
        if (!path.has_extension()) {
            return false;
        }
        using namespace audio_util;
        const auto file_ext = String::ToLower(path.extension().string());
        const auto & support_file_set = GetSupportFileExtensions();
        return support_file_set.find(file_ext) != support_file_set.end();
    }

    XAMP_DISABLE_COPY(TrackInfoExtractAdapterProxy)

    void OnWalkNew() override {
        qApp->processEvents();
    }

    void OnWalk(const Path& path) override {
        qApp->processEvents();
        if (Fs::is_directory(path)) {
            return;
        }
        XAMP_LOG_DEBUG("OnWalk Path: {}", String::ToString(path.wstring()));
        hasher_.Update(path.native());
        paths_.push_front(path);
    }

    void OnWalkEnd(DirectoryEntry const& dir_entry) override {
        qApp->processEvents();
        const auto path_hash = hasher_.GetHash();
        hasher_.Clear();
        
        const auto db_hash = qDatabase.getParentPathHash(QString::fromStdWString(Path(dir_entry).wstring()));
        if (db_hash == path_hash) {
            album_groups_.clear();
            paths_.clear();
            emit adapter_->fromDatabase(qDatabase.getPlayListEntityFromPathHash(db_hash));
            return;
        }

        XAMP_LOG_DEBUG("Cache miss! Hash:{} Path: {}", path_hash, String::ToString(Path(dir_entry).wstring()));

        std::for_each(paths_.begin(), paths_.end(), [&](auto& path) {
            auto metadata = reader_->Extract(path);
			album_groups_[metadata.album].push_front(std::move(metadata));
            });
        std::for_each(album_groups_.begin(), album_groups_.end(), [&](auto& tracks) {
            std::for_each(tracks.second.begin(), tracks.second.end(), [&](auto& track) {
                track.parent_path_hash = path_hash;
                });

            tracks.second.sort([](const auto& first, const auto& last) {
                return first.track < last.track;
                });
        });
        std::for_each(album_groups_.begin(), album_groups_.end(), [&](auto& tracks) {
            emit adapter_->readCompleted(ToTime_t(dir_entry.last_write_time()), tracks.second);
            });
        album_groups_.clear();
        paths_.clear();
    }
	
private:
    AlignPtr<IMetadataReader> reader_;
    QSharedPointer<::MetadataExtractAdapter> adapter_;
    ForwardList<Path> paths_;
    HashMap<std::wstring, ForwardList<TrackInfo>> album_groups_;
    SipHash hasher_;
};

::MetadataExtractAdapter::MetadataExtractAdapter(QObject* parent)
    : QObject(parent) {    
}

void ::MetadataExtractAdapter::readFileMetadata(const QSharedPointer<MetadataExtractAdapter>& adapter, QString const & file_path, bool show_progress_dialog, bool is_recursive) {
	const auto dialog = 
        makeProgressDialog(tr("Read file metadata"),
	    tr("Read progress dialog"), 
	    tr("Cancel"));
    TrackInfoExtractAdapterProxy proxy(adapter);

    QList<QString> dirs;
	QFlags<QDir::Filter> filter = QDir::NoDotDot | QDir::AllDirs;

    if (file_path.back() == L'.') {
        auto temp = file_path;
        filter = QDir::Files;       
        temp = temp.remove(L'.');
        try {
            ScanFolder(temp.toStdWString(), &proxy, false);
        }
        catch (const std::exception& e) {
            XAMP_LOG_DEBUG("ScanFolder has exception: {}", e.what());
        }
        return;
    } else {
        if (show_progress_dialog) {
            dialog->show();
        }

        dialog->setMinimumDuration(1000);
        dialog->setWindowModality(Qt::ApplicationModal);

        filter |= QDir::NoDotAndDotDot | QDir::Files;
        SipHash hasher;

        QStringList name_filter;
        for (auto& file_ext : GetSupportFileExtensions()) {
            name_filter << qSTR("*%1").arg(QString::fromStdString(file_ext));
        }

        QDirIterator itr(file_path, name_filter, filter);
        while (itr.hasNext()) {
            auto path = fromQStringPath(itr.next());
            hasher.Update(path.toStdWString());
            dirs.append(path);
            qApp->processEvents();
        }

        const auto path_hash = hasher.GetHash();
        const auto db_hash = qDatabase.getParentPathHash(file_path);
        if (db_hash == path_hash) {
            XAMP_LOG_DEBUG("Cache Hash:{} Path: {}", db_hash, String::ToString(file_path.toStdWString()));
            emit adapter->fromDatabase(qDatabase.getPlayListEntityFromPathHash(db_hash));
            return;
        }

        if (dirs.isEmpty()) {
            dirs.append(file_path);
        }
    }

    auto progress = 0;
    dialog->setMaximum(dirs.size());

	for (const auto& file_dir_or_path : dirs) {
        if (dialog->wasCanceled()) {
            return;
        }

        dialog->setLabelText(file_dir_or_path);

        try {
            ScanFolder(file_dir_or_path.toStdWString(), &proxy, is_recursive);
        }
        catch (const std::exception& e) {
            XAMP_LOG_DEBUG("ScanFolder has exception: {}", e.what());
        }

        qApp->processEvents();
        dialog->setValue(progress++);
    }
}

#define IgnoreSqlException(expr) \
    do {\
		try {\
			expr;\
		}\
		catch (...) {}\
    } while (false)

void ::MetadataExtractAdapter::addMetadata(const ForwardList<TrackInfo>& result, PlayListTableView* playlist, int64_t dir_last_write_time, bool is_podcast) {
	auto playlist_id = -1;
	if (playlist != nullptr) {
		playlist_id = playlist->playlistId();
	}

	for (const auto& metadata : result) {
		qApp->processEvents();

		auto album = QString::fromStdWString(metadata.album);
		auto artist = QString::fromStdWString(metadata.artist);
		auto disc_id = QString::fromStdString(metadata.disc_id);
     
		auto is_unknown_album = false;
		if (album.isEmpty()) {
			album = tr("Unknown album");
			is_unknown_album = true;
			// todo: 如果有內建圖片就把當作一張專輯.
			auto cover = qDatabaseIdCache.getEmbeddedCover(metadata);
			if (!cover.isNull()) {
				album = QString::fromStdWString(metadata.file_name_no_ext);
			}
		}
		if (artist.isEmpty()) {
			artist = tr("Unknown artist");
		}

		const auto music_id = qDatabase.addOrUpdateMusic(metadata);

		auto [album_id, artist_id, cover_id] = 
            qDatabaseIdCache.addOrGetAlbumAndArtistId(dir_last_write_time,
		             album, 
		             artist,
		             is_podcast, 
		             disc_id);

		if (playlist_id != -1) {
			qDatabase.addMusicToPlaylist(music_id, playlist_id, album_id);
		}

		if (metadata.cover_id.empty()) {
			// Find cover id from database.
			if (cover_id.isEmpty()) {
				cover_id = qDatabase.getAlbumCoverId(album_id);
			}

			// Database not exist find others.
			if (cover_id.isEmpty()) {
				cover_id = qDatabaseIdCache.addCoverCache(album_id, album, metadata, is_unknown_album);
			}
		} else {
			qDatabase.setAlbumCover(album_id, album, QString::fromStdString(metadata.cover_id));
		}

        IgnoreSqlException(qDatabase.addOrUpdateAlbumMusic(album_id,
		                                artist_id,
		                                music_id));
        IgnoreSqlException(qDatabase.addOrUpdateTrackLoudness(album_id,
            artist_id,
            music_id,
            0));
	}

	if (playlist != nullptr) {
		playlist->executeQuery();
	}
}

void ::MetadataExtractAdapter::processMetadata(const ForwardList<TrackInfo>& result, PlayListTableView* playlist, int64_t dir_last_write_time) {
    if (dir_last_write_time == -1) {
        dir_last_write_time = QDateTime::currentSecsSinceEpoch();
    }
    auto is_podcast_mode = false;
    if (playlist != nullptr) {
        is_podcast_mode = playlist->isPodcastMode();
    }
    try {
        qDatabase.transaction();
        addMetadata(result, playlist, dir_last_write_time, is_podcast_mode);
        qDatabase.commit();
    } catch (std::exception const &e) {
        qDatabase.rollback();
        XAMP_LOG_DEBUG("processMetadata throw exception! {}", e.what());
    }
}

TrackInfo getMetadata(QString const& file_path) {
    const Path path(file_path.toStdWString());
    auto reader = MakeMetadataReader();
    return reader->Extract(path);
}
