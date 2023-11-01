#include <widget/backgroundworker.h>

#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/database.h>
#include <widget/databasefacade.h>
#include <widget/mbdiscid_uiltis.h>
#include <widget/http.h>
#include <widget/appsettingnames.h>
#include <widget/read_until.h>
#include <widget/appsettings.h>
#include <widget/widget_shared.h>
#include <widget/imagecache.h>
#include <widget/albumview.h>
#include <widget/tagio.h>

#include <player/ebur128reader.h>
#include <base/logger_impl.h>
#if defined(Q_OS_WIN)
#include <stream/mbdiscid.h>
#endif

#include <QJsonDocument>
#include <QJsonValueRef>
#include <QThread>

XAMP_DECLARE_LOG_NAME(BackgroundWorker);

struct ReplayGainJob {
    PlayListEntity entity;
    Ebur128Reader reader;
};

BackgroundWorker::BackgroundWorker() {    
    logger_ = LoggerManager::GetInstance().GetLogger(kBackgroundWorkerLoggerName);
}

BackgroundWorker::~BackgroundWorker() {
    XAMP_LOG_DEBUG("BackgroundWorker destory!");
}

void BackgroundWorker::OnCancelRequested() {
    is_stop_ = true;
}

void BackgroundWorker::OnSearchLyrics(int32_t music_id, const QString& title, const QString& artist) {
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
        const auto cd = OpenCD(drive.driver_letter);
        cd->SetMaxSpeed();
        const auto tracks = cd->GetTotalTracks();

        auto track_id = 0;
        for (const auto& track : tracks) {
            auto track_info = TagIO::GetTrackInfo(track);
            track_info.file_path = tracks[track_id];
            track_info.duration = cd->GetDuration(track_id++);
            track_info.sample_rate = 44100;
            track_info.disc_id = disc_id;
            track_infos.push_front(track_info);
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
    if (image.isNull()) {
        emit BlurImage(QImage());
        return;
    }    
    emit BlurImage(blur_image_cache_.GetOrAdd(cover_id, [&]() {
        return image_utils::BlurImage(image, size);
        }));
}

void BackgroundWorker::OnReadReplayGain(int32_t playlistId, const QList<PlayListEntity>& entities) {
	const auto writer = MakeMetadataWriter();

    auto entities_size = std::distance(entities.begin(), entities.end());
    XAMP_LOG_D(logger_, "Start read replay gain count:{}", entities_size);

    const auto target_loudness = qAppSettings.GetValue(kAppSettingReplayGainTargetLoudnes).toDouble();
    const auto scan_mode = qAppSettings.ValueAsEnum<ReplayGainScanMode>(kAppSettingReplayGainScanMode);
    const auto enable_write_tag = qAppSettings.ValueAsBool(kAppSettingEnableReplayGainWriteTag);

    QMap<int32_t, Vector<PlayListEntity>> album_group_map;
    for (const auto& entity : entities) {
        album_group_map[entity.album_id].push_back(entity);
    }

    emit ReadFileStart();

    std::atomic<size_t> completed_work(0);
    size_t total_work = 0;
    for (const auto& entities : album_group_map) {
        total_work += entities.size();
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    emit FoundFileCount(total_work);

    for (const auto& entities : album_group_map) {
        Vector<ReplayGainJob> jobs;
        jobs.resize(entities.size());

        for (size_t i = 0; i < entities.size(); ++i) {
            jobs[i].entity = entities[i];
        }

        //std::this_thread::sleep_for(std::chrono::seconds(1));

        Executor::ParallelFor(GetBackgroundThreadPool(), jobs, [this, scan_mode, total_work, &completed_work](auto & job) {
            auto progress = [scan_mode](auto percent) {
                if (scan_mode == ReplayGainScanMode::RG_SCAN_MODE_FAST && percent > 50) {
                    return false;
                }
                return true;
            };

            auto prepare = [&job](auto const& input_format) mutable {
                job.reader.SetSampleRate(input_format.GetSampleRate());
            };

            auto dps_process = [&job, this](const auto * samples, auto sample_size) {
                if (is_stop_) {
                    return;
                }
                job.reader.Process(samples, sample_size);
            };
            read_until::ReadAll(job.entity.file_path.toStdWString(), progress, prepare, dps_process);

        	emit ReadFilePath(job.entity.file_path);

            const auto value = completed_work.load();
            emit ReadFileProgress((value * 100) / total_work);
            ++completed_work;
            });

        ReplayGainResult replay_gain;
        replay_gain.track_peak.reserve(entities.size());
        replay_gain.track_loudness.reserve(entities.size());
        replay_gain.track_gain.reserve(entities.size());
        replay_gain.track_peak_gain.reserve(entities.size());

        for (size_t i = 0; i < entities.size(); ++i) {
            auto track_loudness = jobs[i].reader.GetLoudness();
            auto track_peak = jobs[i].reader.GetTruePeek();
            replay_gain.track_peak.push_back(track_peak);
            replay_gain.track_peak_gain.push_back(20.0 * log10(track_peak));
            replay_gain.track_loudness.push_back(track_loudness);
            replay_gain.track_gain.push_back(target_loudness - track_loudness);
        }

        Vector<Ebur128Reader> readers;
        for (auto& job : jobs) {
            readers.push_back(std::move(job.reader));
        }

        replay_gain.album_loudness = Ebur128Reader::GetMultipleLoudness(readers);

        replay_gain.album_peak = *std::ranges::max_element(replay_gain.track_peak
                                                           ,
                                                           [](auto& a, auto& b) {return a < b; }
        );

        replay_gain.album_gain = target_loudness - replay_gain.album_loudness;
        replay_gain.album_peak_gain = 20.0 * log10(replay_gain.album_peak);

        for (size_t i = 0; i < entities.size(); ++i) {
            if (enable_write_tag) {
                ReplayGain rg;
                rg.album_gain = replay_gain.album_gain;
                rg.track_gain = replay_gain.track_gain[i];
                rg.album_peak = replay_gain.album_peak;
                rg.track_peak = replay_gain.track_peak[i];
                rg.ref_loudness = target_loudness;
                writer->WriteReplayGain(entities[i].file_path.toStdWString(), rg);
            }

            emit ReadReplayGain(playlistId,
                entities[i],
                replay_gain.track_loudness[i],
                replay_gain.album_gain,
                replay_gain.album_peak,
                replay_gain.track_gain[i],
                replay_gain.track_peak[i]
            );
        }
    }

    emit ReadCompleted();
}

void BackgroundWorker::OnTranslation(const QString& keyword, const QString& from, const QString& to) {
    const auto url =
        QString("https://translate.google.com/translate_a/single?client=gtx&sl=%3&tl=%2&dt=t&q=%1")
        .arg(QString(QUrl::toPercentEncoding(keyword)))
        .arg(to)
        .arg(from);
    http::HttpClient(url)
        .success([keyword, this](const QString& content) {
        if (content.isEmpty()) {
            return;
        }
        auto result = content;
        result = result.replace("[[[\"", "");
        result = result.mid(0, result.indexOf(",\"") - 1);
        emit TranslationCompleted(keyword, result);
            }).get();
}