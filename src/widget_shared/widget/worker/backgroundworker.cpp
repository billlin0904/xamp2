#include <widget/worker/backgroundworker.h>

#include <widget/util/str_utilts.h>
#include <widget/util/image_utiltis.h>
#include <widget/database.h>
#include <widget/databasefacade.h>
#include <widget/util/mbdiscid_uiltis.h>
#include <widget/http.h>
#include <widget/appsettingnames.h>
#include <widget/util/read_until.h>
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

struct ReplayGainContext {
    double album_loudness{ 0 };
    double album_peak{ 0 };
    double album_gain{ 0 };
    double album_peak_gain{ 0 };
    Vector<double> track_loudness;
    Vector<double> track_peak;
    Vector<double> track_gain;
    Vector<double> track_peak_gain;
};

BackgroundWorker::BackgroundWorker()
    : nam_(this)
    , buffer_pool_(std::make_shared<ObjectPool<QByteArray>>(256)) {
    logger_ = XampLoggerFactory.GetLogger(kBackgroundWorkerLoggerName);
}

BackgroundWorker::~BackgroundWorker() {
    XAMP_LOG_DEBUG("BackgroundWorker destory!");
}

void BackgroundWorker::cancelRequested() {
    is_stop_ = true;
}

void BackgroundWorker::onSearchLyrics(int32_t music_id, const QString& title, const QString& artist) {
}

#if defined(Q_OS_WIN)
void BackgroundWorker::onFetchCdInfo(const DriveInfo& drive) {
    MBDiscId mbdisc_id;
    std::string disc_id;
    std::string url;

    try {
        disc_id = mbdisc_id.GetDiscId(drive.drive_path.toStdString());
        url = mbdisc_id.GetDiscIdLookupUrl(drive.drive_path.toStdString());
    } catch (const Exception &e) {
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
            auto track_info = TagIO::getTrackInfo(track);
            track_info.file_path = tracks[track_id];
            track_info.duration = cd->GetDuration(track_id++);
            track_info.sample_rate = 44100;
            track_info.disc_id = disc_id;
            track_infos.push_front(track_info);
        }

        track_infos.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
        });

        emit readCdTrackInfo(QString::fromStdString(disc_id), track_infos);
    }
    catch (const Exception& e) {
        XAMP_LOG_DEBUG(e.GetErrorMessage());
        return;
    }

    XAMP_LOG_D(logger_, "Start fetch cd information form musicbrainz.");

    http::HttpClient(QString::fromStdString(url))
        .success([this, disc_id](const auto& url, const auto& content) {
        auto [image_url, mb_disc_id_info] = parseMbDiscIdXml(content);

        mb_disc_id_info.disc_id = disc_id;
        mb_disc_id_info.tracks.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
        });

        emit fetchMbDiscInfoCompleted(mb_disc_id_info);

        XAMP_LOG_D(logger_, "Start fetch cd cover image.");

        http::HttpClient(QString::fromStdString(image_url))
            .success([this, disc_id](const auto& url, const auto& content) {
	            const auto cover_url = parseCoverUrl(content);
                http::HttpClient(cover_url).download([this, disc_id](const auto& content) mutable {
                    QPixmap cover;
                    if (cover.loadFromData(content)) {
	                    const auto cover_id = qImageCache.addImage(cover);
                        XAMP_LOG_D(logger_, "Download cover image completed.");
                        emit fetchDiscCoverCompleted(QString::fromStdString(disc_id), cover_id);
                    }
                    });
                }).get();
            }).get();
}
#endif

void BackgroundWorker::onBlurImage(const QString& cover_id, const QPixmap& image, QSize size) {
    if (image.isNull()) {
        emit blurImage(QImage());
        return;
    }    
    emit blurImage(blur_image_cache_.GetOrAdd(cover_id, [&]() {
        return image_utils::blurImage(image, size);
        }));
}

void BackgroundWorker::onReadReplayGain(int32_t playlistId, const QList<PlayListEntity>& entities) {
	const auto writer = MakeMetadataWriter();

    auto entities_size = std::distance(entities.begin(), entities.end());
    XAMP_LOG_D(logger_, "Start read replay gain count:{}", entities_size);

    const auto target_loudness  = qAppSettings.valueAs(kAppSettingReplayGainTargetLoudnes).toDouble();
    const auto scan_mode        = qAppSettings.valueAsEnum<ReplayGainScanMode>(kAppSettingReplayGainScanMode);
    const auto enable_write_tag = qAppSettings.valueAsBool(kAppSettingEnableReplayGainWriteTag);

    QMap<int32_t, Vector<PlayListEntity>> album_group_map;
    for (const auto& entity : entities) {
        album_group_map[entity.album_id].push_back(entity);
    }

    emit readFileStart();

    std::atomic<size_t> completed_work(0);
    size_t total_work = 0;
    for (const auto& album_entities : album_group_map) {
        total_work += album_entities.size();
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    emit foundFileCount(total_work);

    for (const auto& album_entities : album_group_map) {
        Vector<ReplayGainJob> jobs;
        jobs.resize(entities.size());

        for (size_t i = 0; i < entities.size(); ++i) {
            jobs[i].entity = album_entities[i];
        }

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
            read_until::readAll(job.entity.file_path.toStdWString(), progress, prepare, dps_process);

        	emit readFilePath(job.entity.file_path);

            const auto value = completed_work.load();
            emit readFileProgress((value * 100) / total_work);
            ++completed_work;
            });

        ReplayGainContext replay_gain_context;
        replay_gain_context.track_peak.reserve(entities.size());
        replay_gain_context.track_loudness.reserve(entities.size());
        replay_gain_context.track_gain.reserve(entities.size());
        replay_gain_context.track_peak_gain.reserve(entities.size());

        for (size_t i = 0; i < entities.size(); ++i) {
            auto track_loudness = jobs[i].reader.GetLoudness();
            auto track_peak = jobs[i].reader.GetTruePeek();
            replay_gain_context.track_peak.push_back(track_peak);
            replay_gain_context.track_peak_gain.push_back(20.0 * log10(track_peak));
            replay_gain_context.track_loudness.push_back(track_loudness);
            replay_gain_context.track_gain.push_back(target_loudness - track_loudness);
        }

        Vector<Ebur128Reader> readers;
        for (auto& job : jobs) {
            readers.push_back(std::move(job.reader));
        }

        replay_gain_context.album_loudness = Ebur128Reader::GetMultipleLoudness(readers);
        replay_gain_context.album_peak = *std::max_element(replay_gain_context.track_peak.begin(), 
            replay_gain_context.track_peak.end(), [](auto& a, auto& b) {return a < b; }
        );

        replay_gain_context.album_gain = target_loudness - replay_gain_context.album_loudness;
        replay_gain_context.album_peak_gain = 20.0 * log10(replay_gain_context.album_peak);

        for (size_t i = 0; i < entities.size(); ++i) {
            ReplayGain replay_gain;
            replay_gain.album_gain   = replay_gain_context.album_gain;
            replay_gain.track_gain   = replay_gain_context.track_gain[i];
            replay_gain.album_peak   = replay_gain_context.album_peak;
            replay_gain.track_peak   = replay_gain_context.track_peak[i];
            replay_gain.ref_loudness = target_loudness;

            if (enable_write_tag) {                
                writer->WriteReplayGain(entities[i].file_path.toStdWString(), replay_gain);
            }

            emit readReplayGain(playlistId,
                entities[i],
                replay_gain_context.track_loudness[i],
                replay_gain_context.album_gain,
                replay_gain_context.album_peak,
                replay_gain_context.track_gain[i],
                replay_gain_context.track_peak[i]
            );
        }
    }

    emit readCompleted();
}

void BackgroundWorker::onTranslation(const QString& keyword, const QString& from, const QString& to) {
    const auto url =
        qSTR("https://translate.google.com/translate_a/single?client=gtx&sl=%3&tl=%2&dt=t&q=%1")
        .arg(QString::fromStdString(QUrl::toPercentEncoding(keyword).toStdString()))
        .arg(to)
        .arg(from);
    http::HttpClient(&nam_, buffer_pool_, url)
        .success([keyword, this](const auto& url, const auto& content) {
        if (content.isEmpty()) {
            return;
        }
        auto result = content;
        result = result.replace(qTEXT("[[[\""), qTEXT(""));
        result = result.mid(0, result.indexOf(qTEXT(",\"")) - 1);
        emit translationCompleted(keyword, result);
            }).get();
}
