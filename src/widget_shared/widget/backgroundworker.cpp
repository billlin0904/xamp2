#include <widget/backgroundworker.h>

#include <widget/image_utiltis.h>
#include <widget/database.h>
#include <widget/databasefacade.h>
#include <widget/podcast_uiltis.h>
#include <widget/http.h>
#include <widget/str_utilts.h>
#include <widget/appsettingnames.h>
#include <widget/read_utiltis.h>
#include <widget/appsettings.h>
#include <widget/widget_shared.h>
#include <widget/imagecache.h>
#include <widget/spotify_utilis.h>
#include <widget/colorthief.h>
#include <widget/albumview.h>
#include <widget/kkbox_utilis.h>

#include <player/ebur128reader.h>
#include <base/logger_impl.h>
#include <base/google_siphash.h>
#include <base/scopeguard.h>
#include <base/fastmutex.h>
#include <stream/avfilestream.h>
#if defined(Q_OS_WIN)
#include <stream/mbdiscid.h>
#endif

#include <QJsonDocument>
#include <QJsonArray>
#include <QDirIterator>
#include <QJsonValueRef>
#include <QThread>

#include <thememanager.h>

XAMP_DECLARE_LOG_NAME(BackgroundWorker);

struct ReplayGainJob {
    Vector<PlayListEntity> play_list_entities;
    Vector<Ebur128Reader> scanner;
};

BackgroundWorker::BackgroundWorker() {
    writer_ = MakeMetadataWriter();
    logger_ = LoggerManager::GetInstance().GetLogger(kBackgroundWorkerLoggerName);
}

BackgroundWorker::~BackgroundWorker() {
    XAMP_LOG_DEBUG("BackgroundWorker destory!");
}

void BackgroundWorker::StopThreadPool() {
    is_stop_ = true;
}

void BackgroundWorker::OnLoadAlbumCoverCache() {
    QList<QString> cover_ids;
    cover_ids.reserve(LazyLoadingModel::kMaxBatchSize);

    qDatabase.ForEachAlbumCover([&cover_ids](const auto& cover_id) {
        cover_ids.push_back(cover_id);
        }, LazyLoadingModel::kMaxBatchSize);

    try {
        Executor::ParallelFor(GetBackgroundThreadPool(), cover_ids, [](const auto& cover_id) {
            AlbumViewStyledDelegate::GetCover(AlbumViewStyledDelegate::kAlbumCacheTag, cover_id);
            });
    }
    catch (const std::exception& e) {
		XAMP_LOG_ERROR("OnLoadAlbumCoverCache: {}", e.what());
	}
}

namespace lastfm {
    struct ArtistInfo {
        QString name;
        QString mbid;
        QString url;
        QString image_url;
        QString summary;
    };
}

void BackgroundWorker::OnGetArtist(const QString& artist) {
    //ApiKey = "34f77350f3e23c3a8e0aefecf3ccfaf5";
    //SharedSecret = "bdadfb76384d1d28b078d2ca22e04d95";

    /*const QString searchUrl = qSTR(
        "http://ws.audioscrobbler.com/2.0/?method=artist.getinfo&artist=%1&api_key=%2&format=json"
    ).arg(qTEXT(QUrl::toPercentEncoding(artist))).arg(qTEXT("34f77350f3e23c3a8e0aefecf3ccfaf5"));

    http::HttpClient(searchUrl)
        .success([this, artist](const auto& json) {
        XAMP_LOG_DEBUG("Successfully!");
        QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(json.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            return;
        }
        lastfm::ArtistInfo artist_info;
        artist_info.name = doc["artist"]["name"].toString();
        artist_info.mbid = doc["artist"]["mbid"].toString();
        artist_info.url = doc["artist"]["url"].toString();
        for (auto image : doc["artist"]["image"].toArray()) {
            if (image.toVariant().toMap()["size"].toString() == "mega") {
				artist_info.image_url = image.toVariant().toMap()["#text"].toString();
				break;
			}
		}
		artist_info.summary = doc["artist"]["bio"]["summary"].toString();
        http::HttpClient(artist_info.image_url).download([this, artist](const QByteArray& data) {
            XAMP_LOG_DEBUG("Thread:{} Download podcast image file ({}) success!",
            QThread::currentThreadId(), String::FormatBytes(data.size()));
            emit SearchArtistCompleted(artist, data);
            });
		}).get();*/

    static const QString credentialFLowUrl = qTEXT("https://account.kkbox.com/oauth2/token");
    const QString client_id = qTEXT("bd5cfe4143f918a3db24dcb388972054");
    const QString client_secret = qTEXT("425cacb9f036585d34f7a2725c98e417");
    const QString grant_type = qTEXT("client_credentials");

    http::HttpClient(credentialFLowUrl)
        .param(qTEXT("grant_type"), grant_type)
        .param(qTEXT("client_id"), client_id)
        .param(qTEXT("client_secret"), client_secret)
        .success([this, artist](const auto& json) {
            QJsonParseError error;
            const auto doc = QJsonDocument::fromJson(json.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError) {
                return;
            }
            
            auto access_token = doc[qTEXT("access_token")].toString();
            auto token_type = doc[qTEXT("token_type")].toString();
            auto expires_in = doc[qTEXT("expires_in")].toString();
            
            kkbox::Credential credential;
            credential.access_token = access_token;
            credential.token_type = token_type;
            credential.expires_in = expires_in;

            auto keyword = artist;
            auto type = qTEXT("artist");
            auto territory = qTEXT("JP");

            static const QString searchUrl = qTEXT("https://api.kkbox.com/v1.1/search");
            http::HttpClient(searchUrl)
                .param(qTEXT("q"), keyword)
                .param(qTEXT("type"), type)
                .param(qTEXT("territory"), territory)
                .param(qTEXT("limit"), 15)
                .header(qTEXT("Authorization"), QString(qTEXT("%1 %2").arg(credential.token_type).arg(credential.access_token)))
                .success([this, artist](const auto& json) {
                if (auto artist_data = kkbox::ParseArtistData(json, artist)) {
                    if (artist_data.value().images.size() != 2) {
					    return;
				    }
                    http::HttpClient(artist_data.value().images[1].url)
                        .download([this, artist](const auto &content) {
                        emit SearchArtistCompleted(artist, content);
                        });
			    }			
                }).get();
        })
        .post();
}

void BackgroundWorker::OnSearchLyrics(int32_t music_id, const QString& title, const QString& artist) {
	const auto keywords = QString::fromStdString(
        String::Format("{}{}", 
        QUrl::toPercentEncoding(artist),
        QUrl::toPercentEncoding(title)));

    http::HttpClient(qTEXT("https://music.xianqiao.wang/neteaseapiv2/search"))
        .param(qTEXT("limit"), qTEXT("10"))
        .param(qTEXT("type"), qTEXT("1"))
        .param(qTEXT("keywords"), keywords)
        .success([this, music_id, title](const auto& response) {
			if (response.isEmpty()) {
                return;
            }
			QList<spotify::SearchLyricsResult> results;
            if (!spotify::ParseSearchLyricsResult(response, results)) {
                return;
            }
            auto song_id = 0;
            Q_FOREACH(const auto &result, results) {
                if (result.song != title) {
                   continue;
                }
                song_id = result.id;
                break;
            }
            if (!song_id) {
                emit SearchLyricsCompleted(music_id, kEmptyString, kEmptyString);
                XAMP_LOG_DEBUG("Not found any lyric!");
                return;
            }
            http::HttpClient(qSTR("https://music.xianqiao.wang/neteaseapiv2/lyric?id=%1").arg(song_id))
                .success([this, music_id, title](const auto& resp) {
                const auto [lyrc, trlyrc] = spotify::ParseLyricsResponse(resp);
                emit SearchLyricsCompleted(music_id, lyrc, trlyrc);
            }).get();
		})
        .get();
}

void BackgroundWorker::OnFetchPodcast(int32_t playlist_id) {
    XAMP_LOG_DEBUG("Current thread:{}", QThread::currentThreadId());

    auto download_podcast_error = [this](const QString& msg) {
        XAMP_LOG_DEBUG("Download podcast error! {}", msg.toStdString());
        emit FetchPodcastError(msg);
    };

    http::HttpClient(qTEXT("https://suisei-podcast.outv.im/meta.json"), this)
		.error(download_podcast_error)
        .success([this, download_podcast_error, playlist_id](const QString& json) {
			XAMP_LOG_DEBUG("Thread:{} Download meta.json ({}) success!", 
            QThread::currentThreadId(), String::FormatBytes(json.size()));

        	std::string image_url("https://cdn.jsdelivr.net/gh/suisei-cn/suisei-podcast@0423b62/logo/logo-202108.jpg");

			const Stopwatch watch;
			auto const podcast_info = std::make_pair(image_url, ParseJson(json));
            XAMP_LOG_DEBUG("Thread:{} Parse meta.json success! {:.2f} sesc",
                QThread::currentThreadId(), watch.ElapsedSeconds());

            http::HttpClient(QString::fromStdString(podcast_info.first), this)
                .error(download_podcast_error)
                .download([this, podcast_info, playlist_id](const QByteArray& data) {
					XAMP_LOG_DEBUG("Thread:{} Download podcast image file ({}) success!", 
                    QThread::currentThreadId(), String::FormatBytes(data.size()));
					emit FetchPodcastCompleted(podcast_info.second, data);
				});
            }).get();
}

#if defined(Q_OS_WIN)
void BackgroundWorker::OnFetchCdInfo(const DriveInfo& drive) {
    MBDiscId mbdisc_id;
    std::string disc_id;
    std::string url;

    try {
        disc_id = mbdisc_id.GetDiscId(drive.drive_path.toStdString());
        url = mbdisc_id.GetDiscIdLookupUrl(drive.drive_path.toStdString());
    } catch (Exception const &e) {
        XAMP_LOG_DEBUG(e.GetErrorMessage());
        return;
    }

    try {
	    ForwardList<TrackInfo> track_infos;
	    auto cd = OpenCD(drive.driver_letter);
        cd->SetMaxSpeed();
        const auto tracks = cd->GetTotalTracks();

        auto track_id = 0;
        for (const auto& track : tracks) {
            auto metadata = GetTrackInfo(QString::fromStdWString(track));
            metadata.file_path = tracks[track_id];
            metadata.duration = cd->GetDuration(track_id++);
            metadata.sample_rate = 44100;
            metadata.disc_id = disc_id;
            track_infos.push_front(metadata);
        }

        track_infos.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
        });

        emit OnReadCdTrackInfo(QString::fromStdString(disc_id), track_infos);
    }
    catch (Exception const& e) {
        XAMP_LOG_DEBUG(e.GetErrorMessage());
        return;
    }

    XAMP_LOG_D(logger_, "Start fetch cd information form musicbrainz.");

    http::HttpClient(QString::fromStdString(url))
        .success([this, disc_id](const QString& content) {
        auto [image_url, mb_disc_id_info] = ParseMbDiscIdXml(content);

        mb_disc_id_info.disc_id = disc_id;
        mb_disc_id_info.tracks.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
        });

        emit OnMbDiscInfo(mb_disc_id_info);

        XAMP_LOG_D(logger_, "Start fetch cd cover image.");

        http::HttpClient(QString::fromStdString(image_url))
            .success([this, disc_id](const QString& content) {
	            const auto cover_url = ParseCoverUrl(content);
                http::HttpClient(cover_url).download([this, disc_id](const auto& content) mutable {
                    QPixmap cover;
                    if (cover.loadFromData(content)) {
	                    const auto cover_id = qPixmapCache.AddImage(cover);
                        XAMP_LOG_D(logger_, "Download cover image completed.");
                        emit OnDiscCover(QString::fromStdString(disc_id), cover_id);
                    }
                    });				
                }).get();
            }).get();
}
#endif

void BackgroundWorker::OnBlurImage(const QString& cover_id, const QPixmap& image, QSize size) {
    if (!AppSettings::ValueAsBool(kEnableBlurCover)) {
        emit BlurImage(QImage());
        return;
    }
    ColorThief thief;
    thief.LoadImage(image_utils::ResizeImage(image, QSize(400, 400)).toImage());
    emit DominantColor(thief.GetDominantColor());
    emit BlurImage(image_utils::BlurImage(image, size));
}

void BackgroundWorker::OnReadReplayGain(int32_t playlistId, const ForwardList<PlayListEntity>& entities) {
    auto entities_size = std::distance(entities.begin(), entities.end());
    XAMP_LOG_D(logger_, "Start read replay gain count:{}", entities_size);

    const auto target_loudness = AppSettings::GetValue(kAppSettingReplayGainTargetLoudnes).toDouble();
    const auto scan_mode = AppSettings::ValueAsEnum<ReplayGainScanMode>(kAppSettingReplayGainScanMode);

    QMap<int32_t, Vector<PlayListEntity>> album_group_map;
    for (const auto& entity : entities) {
        album_group_map[entity.album_id].push_back(entity);
    }

    for (const auto& album : album_group_map) {
        FastMutex mutex;
        ReplayGainJob jobs;

        Executor::ParallelFor(GetBackgroundThreadPool(), album, [this, scan_mode, &mutex, &jobs](auto const& entity) {
            auto progress = [scan_mode](auto percent) {
                if (scan_mode == ReplayGainScanMode::RG_SCAN_MODE_FAST && percent > 50) {
                    return false;
                }
                return true;
            };
			Ebur128Reader scanner;
            auto prepare = [&scanner](auto const& input_format) mutable {
                scanner.SetSampleRate(input_format.GetSampleRate());
            };
            auto dps_process = [&scanner, this](auto const* samples, auto sample_size) {
                if (is_stop_) {
                    return;
                }
                scanner.Process(samples, sample_size);
            };
            read_utiltis::ReadAll(entity.file_path.toStdWString(), progress, prepare, dps_process);
            std::lock_guard<FastMutex> guard{ mutex };
            jobs.play_list_entities.push_back(entity);
            jobs.scanner.push_back(std::move(scanner));
            });

        if (album.size() != jobs.play_list_entities.size()) {
            XAMP_LOG_DEBUG("Abnormal completed completed tacks:{} all songs:{}",
                jobs.play_list_entities.size(),
                album.size());
            continue;
        }

        ReplayGainResult replay_gain;
        replay_gain.track_peak.reserve(album.size());
        replay_gain.track_loudness.reserve(album.size());
        replay_gain.track_gain.reserve(album.size());
        replay_gain.track_peak_gain.reserve(album.size());
        replay_gain.album_loudness = Ebur128Reader::GetMultipleLoudness(jobs.scanner);

        for (size_t i = 0; i < album.size(); ++i) {
            auto track_loudness = jobs.scanner[i].GetLoudness();
            auto track_peak = jobs.scanner[i].GetTruePeek();
            replay_gain.track_peak.push_back(track_peak);
            replay_gain.track_peak_gain.push_back(20.0 * log10(track_peak));
            replay_gain.track_loudness.push_back(track_loudness);
            replay_gain.track_gain.push_back(target_loudness - track_loudness);
        }

        replay_gain.album_peak = *std::max_element(
            replay_gain.track_peak.begin(),
            replay_gain.track_peak.end(),
            [](auto& a, auto& b) {return a < b; }
        );

        replay_gain.album_gain = target_loudness - replay_gain.album_loudness;
        replay_gain.album_peak_gain = 20.0 * log10(replay_gain.album_peak);
        replay_gain.play_list_entities = jobs.play_list_entities;

        for (size_t i = 0; i < album.size(); ++i) {
            ReplayGain rg;
            rg.album_gain = replay_gain.album_gain;
            rg.track_gain = replay_gain.track_gain[i];
            rg.album_peak = replay_gain.album_peak;
            rg.track_peak = replay_gain.track_peak[i];
            rg.ref_loudness = target_loudness;

            writer_->WriteReplayGain(replay_gain.play_list_entities[i].file_path.toStdWString(), rg);
            emit ReadReplayGain(playlistId,
                replay_gain.play_list_entities[i],
                replay_gain.track_loudness[i],
                replay_gain.album_gain,
                replay_gain.album_peak,
                replay_gain.track_gain[i],
                replay_gain.track_peak[i]
            );
        }
    }
}

void BackgroundWorker::OnExtractFile(const QString& file_path, int32_t playlist_id, bool is_podcast_mode) {
    const auto facade = QSharedPointer<DatabaseFacade>(new DatabaseFacade());
    facade->ReadTrackInfo(this, file_path, playlist_id, false);
}