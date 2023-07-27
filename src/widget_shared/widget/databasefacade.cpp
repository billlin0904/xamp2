#include <widget/databasefacade.h>
#include <widget/extractfileworker.h>

#include <atomic>
#include <execution>

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
        pixmap.loadFromData(buffer.data(), static_cast<uint32_t>(buffer.size()));
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
    static const QString kJop = "jpop";
    QStringList normalizedTags;

    if (genre.isEmpty()) return normalizedTags;

    if (genre.length() == 1 && genre[0] == ' ') return normalizedTags;

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
                normalizedTags.append(s);
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
            normalizedTags.append(s);
        }
    }

    return normalizedTags;
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
    constexpr ConstLatin1String kHiRes("hires");    
    constexpr ConstLatin1String kDsdCategory("dsd");
    constexpr auto k24Bit96KhzBitRate = 4608;

    const std::wstring kDffExtension(L".dff");
    const std::wstring kDsfExtension(L".dsf");

    FloatMap<QString, int32_t> artist_id_cache;
    FloatMap<QString, int32_t> album_id_cache;
    FloatMap<int32_t, QString> cover_id_cache;
    CoverArtReader reader;
    
    auto album_year = result.front().year;
    auto album_genre = NormalizeGenre(QString::fromStdWString(result.front().genre)).join(",");
    
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

        const auto music_id = qDatabase.AddOrUpdateMusic(track_info);
        int32_t artist_id = 0;
        if (!artist_id_cache.contains(artist)) {
            artist_id = qDatabase.AddOrUpdateArtist(artist);
            artist_id_cache[artist] = artist_id;
        } else {
            artist_id = artist_id_cache[artist];
        }

        XAMP_EXPECTS(artist_id != 0);

        auto album_id = qDatabase.GetAlbumId(album);
        if (album_id == kInvalidDatabaseId) {
            album_id = qDatabase.AddOrUpdateAlbum(album,
                artist_id,
                track_info.last_write_time,
                album_year,
                is_podcast,
                disc_id,
                album_genre);
            album_id_cache[album] = album_id;
            for (const auto &category : GetAlbumCategories(album)) {
                qDatabase.AddOrUpdateAlbumCategory(album_id, category);
            }
            
            if (track_info.file_ext == kDsfExtension || track_info.file_ext == kDffExtension) {
                qDatabase.AddOrUpdateAlbumCategory(album_id, kDsdCategory);
            }
            else if (track_info.bit_rate >= k24Bit96KhzBitRate) {
                qDatabase.AddOrUpdateAlbumCategory(album_id, kHiRes);
			}            
        }

        XAMP_EXPECTS(album_id != 0);

		if (playlist_id != -1) {
            qDatabase.AddMusicToPlaylist(music_id, playlist_id, album_id);
		}

        qDatabase.AddOrUpdateAlbumMusic(album_id, artist_id, music_id);
        qDatabase.AddOrUpdateAlbumArtist(album_id, artist_id);
        for (const auto& multi_artist : artists) {
            auto id = qDatabase.AddOrUpdateArtist(multi_artist);
            qDatabase.AddOrUpdateAlbumArtist(album_id, id);
        }

        if (!is_file_path) {
            continue;
        }        

        if (!cover.isNull()) {
            qDatabase.SetAlbumCover(album_id, album, qPixmapCache.AddImage(cover));
            continue;
        }

        if (cover_id_cache.contains(album_id)) {
            continue;
        }

        const auto cover_id = qDatabase.GetAlbumCoverId(album_id);
        if (cover_id.isEmpty()) {
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
        if (!qDatabase.transaction()) {
            XAMP_LOG_DEBUG("Failed to begin transaction!");
            return;
        }
        AddTrackInfo(result, playlist_id, is_podcast_mode);       
        if (!qDatabase.commit()) {
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
    qDatabase.rollback();
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
