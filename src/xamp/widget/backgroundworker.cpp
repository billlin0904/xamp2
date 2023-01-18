#include <widget/backgroundworker.h>

#include <widget/image_utiltis.h>
#include <widget/database.h>
#include <widget/metadataextractadapter.h>
#include <widget/podcast_uiltis.h>
#include <widget/http.h>
#include <widget/str_utilts.h>
#include <widget/appsettingnames.h>
#include <widget/ui_utilts.h>
#include <widget/read_utiltis.h>
#include <widget/appsettings.h>
#include <widget/widget_shared.h>
#include <widget/pixmapcache.h>
#include <widget/spotify_utilis.h>

#include <player/ebur128reader.h>
#include <base/logger_impl.h>
#include <base/google_siphash.h>
#include <base/scopeguard.h>
#if defined(Q_OS_WIN)
#include <player/mbdiscid.h>
#endif

#include <QDirIterator>
#include <QThread>

XAMP_DECLARE_LOG_NAME(BackgroundThreadPool);
XAMP_DECLARE_LOG_NAME(BackgroundWorker);

struct ReplayGainJob {
    Vector<PlayListEntity> play_list_entities;
    Vector<Ebur128Reader> scanner;
};

using DirPathHash = GoogleSipHash<>;

static void ScanDirFiles(const QSharedPointer<DatabaseProxy>& adapter,
    const QStringList& file_name_filters,
    const QString& dir,
    int32_t playlist_id,
    bool is_podcast_mode) {
    QDirIterator itr(dir, file_name_filters, 
        QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);

    DirPathHash hasher(DatabaseProxy::kDirHashKey1, DatabaseProxy::kDirHashKey2);
    ForwardList<Path> paths;

    while (itr.hasNext()) {
        auto next_path = toNativeSeparators(itr.next());
        auto path = next_path.toStdWString();
        paths.push_front(path);
        hasher.Update(path);
    }

    if (paths.empty()) {
        paths.push_front(dir.toStdWString());
    }

    const auto path_hash = hasher.GetHash();

    const auto db_hash = qDatabase.getParentPathHash(dir);
    if (db_hash == path_hash) {
        XAMP_LOG_DEBUG("Cache hit hash:{} path: {}", db_hash, String::ToString(dir.toStdWString()));
        emit adapter->fromDatabase(qDatabase.getPlayListEntityFromPathHash(db_hash));
        return;
    }

    auto reader = MakeMetadataReader();

    HashMap<std::wstring, ForwardList<TrackInfo>> album_groups;
    std::for_each(paths.begin(), paths.end(), [&](auto& path) {
        auto track_info = reader->Extract(path);
		album_groups[track_info.album].push_front(std::move(track_info));
    });

    std::for_each(album_groups.begin(), album_groups.end(), [&](auto& tracks) {
        std::for_each(tracks.second.begin(), tracks.second.end(), [&](auto& track) {
            track.parent_path_hash = path_hash;
        });

		tracks.second.sort([](const auto& first, const auto& last) {
			return first.track < last.track;
        });
    });

    const DirectoryEntry dir_entry(dir.toStdWString());
    std::for_each(album_groups.begin(), album_groups.end(), [&](auto& album_tracks) {
		DatabaseProxy::insertTrackInfo(album_tracks.second, playlist_id, is_podcast_mode);
		emit adapter->readCompleted(album_tracks.second);
    });
}

BackgroundWorker::BackgroundWorker() {
    writer_ = MakeMetadataWriter();
    logger_ = LoggerManager::GetInstance().GetLogger(kBackgroundWorkerLoggerName);
}

BackgroundWorker::~BackgroundWorker() = default;

void BackgroundWorker::stopThreadPool() {
    is_stop_ = true;
    if (executor_ != nullptr) {
        executor_->Stop();
    }
}

void BackgroundWorker::lazyInitExecutor() {
    if (executor_ != nullptr) {
        return;
    }

    CpuAffinity affinity;
    affinity.Set(2);
    affinity.Set(3);
    executor_ = MakeThreadPoolExecutor(kBackgroundThreadPoolLoggerName,
        ThreadPriority::BACKGROUND, 
        affinity);
}

void BackgroundWorker::onProcessImage(const QString& file_path, const QByteArray& buffer, const QString& tag_name) {
    lazyInitExecutor();

    executor_->Spawn([file_path, tag_name, buffer, this]() {
        if (is_stop_) {
            return;
        }

        try {
            qPixmapCache.optimizeImageFromBuffer(file_path, buffer, tag_name);
        } catch (std::exception const &e) {
            XAMP_LOG_E(logger_, "Faild to optimize image. {}", e.what());
        }
    });
}

void BackgroundWorker::onReadTrackInfo(const QSharedPointer<DatabaseProxy>& adapter,
    QString const& file_path,
    int32_t playlist_id,
    bool is_podcast_mode) {
    lazyInitExecutor();

    constexpr QFlags<QDir::Filter> filter = QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs;
    QDirIterator itr(file_path, getFileNameFilter(), filter);
    DirPathHash hasher(DatabaseProxy::kDirHashKey1, DatabaseProxy::kDirHashKey2);

    Vector<QString> dirs;

    while (itr.hasNext()) {
        if (is_stop_) {
            return;
        }
        auto path = toNativeSeparators(itr.next());
        hasher.Update(path.toStdWString());
        dirs.push_back(path);
    }

    if (dirs.empty()) {
        dirs.push_back(file_path);
    }

    const int dir_size = std::distance(dirs.begin(), dirs.end());
    emit adapter->readFileStart(dir_size);

    XAMP_ON_SCOPE_EXIT(
        emit adapter->readFileEnd();
		XAMP_LOG_D(logger_, "Finish to read track info.");
    );

    const auto path_hash = hasher.GetHash();

    try {
        const auto db_hash = qDatabase.getParentPathHash(toNativeSeparators(file_path));
        if (db_hash == path_hash) {
            XAMP_LOG_D(logger_, "Cache hit hash:{} path: {}", db_hash, String::ToString(file_path.toStdWString()));
            emit adapter->fromDatabase(qDatabase.getPlayListEntityFromPathHash(db_hash));
            return;
        }
    } catch (Exception const &e) {
        XAMP_LOG_E(logger_, "Faild to get parent path. {}", e.GetErrorMessage());
        return;
    }    

    const auto file_name_filters = getFileNameFilter();
    std::atomic<int> progress(0);

    Executor::ParallelFor(*executor_, dirs, [this, adapter, &progress, &file_name_filters, playlist_id, is_podcast_mode, dir_size](
        const auto& dir) {
    	if (is_stop_) {
            return;
        }

		try {
			ScanDirFiles(adapter, file_name_filters, dir, playlist_id, is_podcast_mode);
		}
		catch (Exception const& e) {
			XAMP_LOG_D(logger_, "Faild to scan dir files! ", e.GetErrorMessage());
		}
        emit adapter->readFileProgress(dir, progress);
		++progress;
    });
}

void BackgroundWorker::onSearchLyrics(const QString& title, const QString& artist) {
	const auto keywords = QString::fromStdString(
        String::Format("{} {}", 
        QUrl::toPercentEncoding(artist),
        QUrl::toPercentEncoding(title)));

    http::HttpClient(qTEXT("https://music.xianqiao.wang/neteaseapiv2/search"))
        .param(qTEXT("limit"), qTEXT("10"))
        .param(qTEXT("type"), qTEXT("1"))
        .param(qTEXT("keywords"), keywords)
        .success([this](const QString& response) {
			if (response.isEmpty()) {
                return;
            }
        	Spotify::SearchLyricsResult result;
            Spotify::parseJson(response, result);
		})
        .get();
}

void BackgroundWorker::onFetchPodcast(int32_t playlist_id) {
    XAMP_LOG_DEBUG("Current thread:{}", QThread::currentThreadId());

    auto download_podcast_error = [this](const QString& msg) {
        XAMP_LOG_DEBUG("Download podcast error! {}", msg.toStdString());
        emit fetchPodcastError(msg);
    };

    http::HttpClient(qTEXT("https://suisei-podcast.outv.im/meta.json"), this)
		.error(download_podcast_error)
        .success([this, download_podcast_error, playlist_id](const QString& json) {
			XAMP_LOG_DEBUG("Thread:{} Download meta.json ({}) success!", 
            QThread::currentThreadId(), String::FormatBytes(json.size()));

        	std::string image_url("https://cdn.jsdelivr.net/gh/suisei-cn/suisei-podcast@0423b62/logo/logo-202108.jpg");

            Stopwatch watch;
			auto const podcast_info = std::make_pair(image_url, parseJson(json));
            XAMP_LOG_DEBUG("Thread:{} Parse meta.json success! {:.2f} sesc",
                QThread::currentThreadId(), watch.ElapsedSeconds());

            http::HttpClient(QString::fromStdString(podcast_info.first), this)
                .error(download_podcast_error)
                .download([this, podcast_info, playlist_id](const QByteArray& data) {
					XAMP_LOG_DEBUG("Thread:{} Download podcast image file ({}) success!", 
                    QThread::currentThreadId(), String::FormatBytes(data.size()));
					::DatabaseProxy::insertTrackInfo(podcast_info.second, playlist_id, true);
					emit fetchPodcastCompleted(podcast_info.second, data);
				});
            }).get();
}

void BackgroundWorker::onFetchCdInfo(const DriveInfo& drive) {
#if defined(Q_OS_WIN)
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
            auto metadata = getTrackInfo(QString::fromStdWString(track));
            metadata.file_path = tracks[track_id];
            metadata.duration = cd->GetDuration(track_id++);
            metadata.sample_rate = 44100;
            metadata.disc_id = disc_id;
            track_infos.push_front(metadata);
        }

        track_infos.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });

        emit updateCdMetadata(QString::fromStdString(disc_id), track_infos);
    }
    catch (Exception const& e) {
        XAMP_LOG_DEBUG(e.GetErrorMessage());
        return;
    }

    XAMP_LOG_D(logger_, "Start fetch cd information form musicbrainz.");

    http::HttpClient(QString::fromStdString(url))
        .success([this, disc_id](const QString& content) {
        auto [image_url, mb_disc_id_info] = parseMbDiscIdXML(content);

        mb_disc_id_info.disc_id = disc_id;
        mb_disc_id_info.tracks.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });

        emit updateMbDiscInfo(mb_disc_id_info);

        XAMP_LOG_D(logger_, "Start fetch cd cover image.");

        http::HttpClient(QString::fromStdString(image_url))
            .success([this, disc_id](const QString& content) {
	            const auto cover_url = parseCoverUrl(content);
                http::HttpClient(cover_url).download([this, disc_id](const auto& content) mutable {
                    auto cover_id = qPixmapCache.addOrUpdate(content);
					XAMP_LOG_D(logger_, "Download cover image completed.");
                    emit updateDiscCover(QString::fromStdString(disc_id), cover_id);
                    });				
                }).get();
            }).get();
#endif
}

void BackgroundWorker::onBlurImage(const QString& cover_id, const QPixmap& image, QSize size) {
    if (!AppSettings::getValueAsBool(kEnableBlurCover)) {
        emit updateBlurImage(QImage());
        return;
    }
    emit updateBlurImage(ImageUtils::blurImage(image, size));
}

void BackgroundWorker::onReadReplayGain(int32_t playlistId, const ForwardList<PlayListEntity>& entities) {
    lazyInitExecutor();

    auto entities_size = std::distance(entities.begin(), entities.end());
    XAMP_LOG_D(logger_, "Start read replay gain count:{}", entities_size);

    const auto target_loudness = AppSettings::getValue(kAppSettingReplayGainTargetLoudnes).toDouble();
    const auto scan_mode = AppSettings::getAsEnum<ReplayGainScanMode>(kAppSettingReplayGainScanMode);

    QMap<int32_t, Vector<PlayListEntity>> album_group_map;
    for (const auto& entity : entities) {
        album_group_map[entity.album_id].push_back(entity);
    }

    for (const auto& album : album_group_map) {
        FastMutex mutex;
        ReplayGainJob jobs;

        Executor::ParallelFor(*executor_, album, [this, scan_mode, &mutex, &jobs](auto const &entity) {
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
            read_utiltis::readAll(entity.file_path.toStdWString(), progress, prepare, dps_process);
            std::lock_guard<FastMutex> guard{ mutex };
            jobs.play_list_entities.push_back(entity);
            jobs.scanner.push_back(std::move(scanner));
            });

        if (album.size() != jobs.play_list_entities.size()) {
            XAMP_LOG_DEBUG("Abnormal completed completed tacks:{} all tracks:{}",
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
            emit updateReplayGain(playlistId,
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
