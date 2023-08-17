#include <widget/databasefacade.h>
#include <widget/extractfileworker.h>

#include <atomic>
#include <execution>

#include <QRegExp>

#include <qguiapplication.h>

#include <base/base.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>

#include <metadata/api.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadataextractadapter.h>
#include <stream/avfilestream.h>

#include <widget/widget_shared.h>
#include <widget/http.h>
#include <widget/xmessagebox.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/imagecache.h>

XAMP_DECLARE_LOG_NAME(DatabaseFacade);

#define IGNORE_ANY_EXCEPTION(expr) \
    do {\
		try {\
			expr;\
		}\
		catch (...) {}\
    } while (false)

static QSet<QString> GetAlbumCategories(const QString& album) {
    QRegularExpression regex(
        R"((final fantasy \b|piano|vocal|soundtrack|best|complete|collection|edition|version|the king of fighter)(?:(?: \[.*\])|(?: - .*))?)",
        QRegularExpression::CaseInsensitiveOption);

    QSet<QString> categories;
    auto it = regex.globalMatch(album);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString category = match.captured(1).toLower().trimmed();
        categories.insert(category);
    }
    return categories;
}

CoverArtReader::CoverArtReader()
    : cover_reader_(MakeMetadataReader()) {
}

QPixmap CoverArtReader::GetEmbeddedCover(const Path& file_path) const {
    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(), buffer.size());
    }
    return pixmap;
}

QPixmap CoverArtReader::GetEmbeddedCover(const TrackInfo& track_info) const {
    return GetEmbeddedCover(track_info.file_path);
}

DatabaseFacade::DatabaseFacade(QObject* parent)
    : QObject(parent) {
    logger_ = LoggerManager::GetInstance().GetLogger(kDatabaseFacadeLoggerName);    
}

QStringList DatabaseFacade::NormalizeGenre(const QString& genre) {
    static constexpr auto kJop = qTEXT("jpop");
    QStringList normalized_tags;

    /*if (genre.isEmpty()) return normalized_tags;

    if (genre.length() == 1 && genre[0] == ' ') return normalized_tags;

    auto tags = genre.split(QRegExp("\\s*,\\s*"), Qt::SkipEmptyParts);

    for (auto tag : tags) {
        tag = tag.trimmed();
        if (tag.contains(QRegExp("[/|()&]"))) {
            QStringList subTags = tag.split(QRegExp("[/|()&]"), Qt::SkipEmptyParts);
            for (auto s : subTags) {         
                s = s.trimmed().toLower();
                if (s.length() == 1) {
                    continue;
                }                
                if (s == kJop) {
                    s = "j-pop";
                }
                normalized_tags.append(s);
            }            
        }
        else {
            auto s = tag.trimmed().toLower();
            if (s.length() == 1) {
                continue;
            }
            if (s == kJop) {
                s = "j-pop";
            }
            normalized_tags.append(s);
        }
    }*/

    return normalized_tags;
}

void NormalizeArtist(QString &artist, QStringList &artists) {
    if (artist.contains(' ')) {
        if (!artist[0].isUpper()) {
            artist = artist.remove(' ');
        }
        else {
            artist = artist.trimmed();
        }
    }

    artists = artist.split(QRegularExpression("[,/&]"), Qt::SkipEmptyParts);
    if (!artists.isEmpty()) {
        artist = artists.first();
        artists.pop_front();
    }
}

void DatabaseFacade::AddTrackInfo(const Vector<TrackInfo>& result,
    int32_t playlist_id,
    bool is_podcast) {
    const Stopwatch sw;
    static constexpr auto kHiRes = qTEXT("HiRes");
    static constexpr auto kDsdCategory = qTEXT("DSD");
    static constexpr auto k24Bit96KhzBitRate = 4608;

    const std::wstring kDffExtension(L".dff");
    const std::wstring kDsfExtension(L".dsf");

    FloatMap<QString, int32_t> artist_id_cache;
    FloatMap<QString, int32_t> album_id_cache;
    FloatMap<int32_t, QString> cover_id_cache;
    HashSet<bool> find_cover_state_cache;
    const CoverArtReader reader;

    const auto album_year = result.front().year;
    const auto album_genre = NormalizeGenre(QString::fromStdWString(result.front().genre)).join(",");
    
	for (const auto& track_info : result) {        
        auto file_path = QString::fromStdWString(track_info.file_path);
        auto album = QString::fromStdWString(track_info.album);
        auto artist = QString::fromStdWString(track_info.artist);
		auto disc_id = QString::fromStdString(track_info.disc_id);

        QStringList artists;
        NormalizeArtist(artist, artists);
                
        const auto is_file_path = IsFilePath(track_info.file_path);

        QPixmap cover;
		if (is_file_path && album.isEmpty()) {
			album = tr("Unknown album");
			// todo: 如果有內建圖片就把當作一張專輯.
			cover = reader.GetEmbeddedCover(track_info);
			if (!cover.isNull()) {
				album = QString::fromStdWString(track_info.file_name_no_ext);
			}
		}

		if (artist.isEmpty() || is_podcast) {
			artist = tr("Unknown artist");
		}

        const auto music_id = qMainDb.AddOrUpdateMusic(track_info);
        int32_t artist_id = 0;
        if (!artist_id_cache.contains(artist)) {
            artist_id = qMainDb.AddOrUpdateArtist(artist);
            artist_id_cache[artist] = artist_id;
        } else {
            artist_id = artist_id_cache[artist];
        }

        XAMP_EXPECTS(artist_id != 0);

        auto album_id = qMainDb.GetAlbumId(album);
        if (album_id == kInvalidDatabaseId) {
            album_id = qMainDb.AddOrUpdateAlbum(album,
                artist_id,
                track_info.last_write_time,
                album_year,
                is_podcast,
                disc_id,
                album_genre);
            album_id_cache[album] = album_id;
            for (const auto &category : GetAlbumCategories(album)) {
                qMainDb.AddOrUpdateAlbumCategory(album_id, category);
            }
            
            if (track_info.file_ext == kDsfExtension || track_info.file_ext == kDffExtension) {
                qMainDb.AddOrUpdateAlbumCategory(album_id, kDsdCategory);
            }
            else if (track_info.bit_rate >= k24Bit96KhzBitRate) {
                qMainDb.AddOrUpdateAlbumCategory(album_id, kHiRes);
			}            
        }

        XAMP_EXPECTS(album_id != 0);

		if (playlist_id != -1) {
            qMainDb.AddMusicToPlaylist(music_id, playlist_id, album_id);
		}

        qMainDb.AddOrUpdateAlbumMusic(album_id, artist_id, music_id);
        qMainDb.AddOrUpdateAlbumArtist(album_id, artist_id);
        for (const auto& multi_artist : artists) {
            auto id = qMainDb.AddOrUpdateArtist(multi_artist);
            qMainDb.AddOrUpdateAlbumArtist(album_id, id);
        }

        if (!is_file_path) {
            continue;
        }

        if (!cover.isNull()) {
            qMainDb.SetAlbumCover(album_id, album, qPixmapCache.AddImage(cover));
            continue;
        }

        if (cover_id_cache.contains(album_id)) {
            continue;
        }

        if (find_cover_state_cache.contains(album_id)) {
            continue;
        }

        const auto cover_id = qMainDb.GetAlbumCoverId(album_id);
        if (cover_id.isEmpty()) {
            find_cover_state_cache.insert(album_id);
            emit FindAlbumCover(album_id, album, track_info.file_path);
        }
        else {
            cover_id_cache[album_id] = cover_id;
        }
	}
    if (sw.ElapsedSeconds() > 1.0) {
        XAMP_LOG_DEBUG("AddTrackInfo ({} secs, {} items)", sw.ElapsedSeconds(), result.size());
    }
}

void DatabaseFacade::InsertTrackInfo(const Vector<TrackInfo>& result,
    int32_t playlist_id, 
    bool is_podcast_mode) {
    try {
        if (!qMainDb.transaction()) {
            XAMP_LOG_DEBUG("Failed to begin transaction!");
            return;
        }
        AddTrackInfo(result, playlist_id, is_podcast_mode);       
        if (!qMainDb.commit()) {
            XAMP_LOG_DEBUG("Failed to commit!");
        }
        return;
    }
    catch (Exception const& e) {
        XAMP_LOG_DEBUG("Failed to add track info({})!", e.GetErrorMessage());
    }
    catch (std::exception const& e) {
        XAMP_LOG_DEBUG("Failed to add track info({})!", e.what());
    }
    catch (...) {
        XAMP_LOG_DEBUG("Failed to add track info!");
    }
    qMainDb.rollback();
}

TrackInfo GetTrackInfo(QString const& file_path) {
    const Path path(file_path.toStdWString());
    const auto reader = MakeMetadataReader();
    return reader->Extract(path);
}

const QStringList& GetFileNameFilter() {
    struct StaticGetFileNameFilter {
        StaticGetFileNameFilter() {
            for (auto& file_ext : GetSupportFileExtensions()) {
                name_filter << qSTR("*%1").arg(QString::fromStdString(file_ext));
            }
        }
        QStringList name_filter;
    };
    return SharedSingleton<StaticGetFileNameFilter>::GetInstance().name_filter;
}
