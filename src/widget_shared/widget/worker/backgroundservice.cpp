#include <widget/worker/backgroundservice.h>

#include <widget/util/str_util.h>
#include <widget/util/image_util.h>
#include <widget/database.h>
#include <widget/databasefacade.h>
#include <widget/util/mbdiscid_util.h>
#include <widget/appsettingnames.h>
#include <widget/util/read_until.h>
#include <widget/appsettings.h>
#include <widget/widget_shared.h>
#include <widget/imagecache.h>
#include <widget/albumview.h>
#include <widget/tagio.h>
#include <widget/util/read_until.h>

#include <base/logger_impl.h>

#if defined(Q_OS_WIN)
#include <stream/mbdiscid.h>
#endif

#include <QDir>
#include <QJsonValueRef>
#include <QThread>

XAMP_DECLARE_LOG_NAME(BackgroundService);

BackgroundService::BackgroundService()
    : nam_(this)
	, http_client_(&nam_, QString(), this)
    , buffer_pool_(MakeObjectPool<QByteArray>(kBufferPoolSize)) {
    logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(BackgroundService));
    thread_pool_ = ThreadPoolBuilder::MakeBackgroundThreadPool();
}

BackgroundService::~BackgroundService() = default;

void BackgroundService::cancelAllJob() {
    stop_source_.request_stop();
}

void BackgroundService::onAddJobs(const QString& dir_name, QList<EncodeJob> jobs) {
    stop_source_ = std::stop_source();
    
    /*Executor::ParallelFor(thread_pool_.get(), stop_source_.get_token(), jobs, [dir_name, this](auto &job) {
        const auto save_file_name = dir_name + "/"_str + job.file.file_name + ".m4a"_str;
        auto encoder = StreamFactory::MakeAlacEncoder();
        Path output_path(save_file_name.toStdWString());
        AnyMap config;
        config.AddOrReplace(FileEncoderConfig::kInputFilePath, Path(job.file.file_path.toStdWString()));
        config.AddOrReplace(FileEncoderConfig::kOutputFilePath, output_path);
        encoder->Start(config);
        encoder->Encode([job, this](auto progress) {
            if (progress % 10 == 0) {
                emit updateJobProgress(job.job_id, progress);
            }
            return true;
            });
        encoder.reset();
        TagIO tag_io;
        tag_io.writeArtist(output_path, job.file.artist);
        tag_io.writeTitle(output_path, job.file.title);
        tag_io.writeAlbum(output_path, job.file.album);
        tag_io.writeComment(output_path, job.file.comment);
        tag_io.writeGenre(output_path, job.file.genre);
        tag_io.writeTrack(output_path, job.file.track);
        tag_io.writeYear(output_path, job.file.year);
        emit updateJobProgress(job.job_id, 100);
        });*/
    for (const auto& job : jobs) {
        if (stop_source_.stop_requested()) {
            return;
        }

        const auto temp_file_name = QDir::tempPath() + "/"_str + job.file.file_name + ".m4a"_str;
        const auto save_file_name = dir_name + "/"_str + job.file.file_name + ".m4a"_str;

        try {
            auto encoder = StreamFactory::MakeAlacEncoder();
            Path output_path(temp_file_name.toStdWString());
            AnyMap config;
            config.AddOrReplace(FileEncoderConfig::kInputFilePath, Path(job.file.file_path.toStdWString()));
            config.AddOrReplace(FileEncoderConfig::kOutputFilePath, output_path);
            encoder->Start(config);
            encoder->Encode([job, this](auto progress) {
                if (progress % 10 == 0) {
                    emit updateJobProgress(job.job_id, progress);
                }
                return true;
                });
            encoder.reset();

            TagIO tag_io;
            tag_io.writeArtist(output_path, job.file.artist);
            tag_io.writeTitle(output_path, job.file.title);
            tag_io.writeAlbum(output_path, job.file.album);
            tag_io.writeComment(output_path, job.file.comment);
            tag_io.writeGenre(output_path, job.file.genre);
            tag_io.writeTrack(output_path, job.file.track);
            tag_io.writeYear(output_path, job.file.year);
            QFile::rename(temp_file_name, save_file_name);
            emit updateJobProgress(job.job_id, 100);
            continue;
        }
        catch (const std::exception &e) {
            XAMP_LOG_DEBUG(e.what());
        }
        QFile::remove(temp_file_name);
    }
}

void BackgroundService::cancelRequested() {
    is_stop_ = true;
}

void BackgroundService::onSearchLyrics(int32_t music_id, const QString& title, const QString& artist) {
}

#if defined(Q_OS_WIN)
void BackgroundService::onFetchCdInfo(const DriveInfo& drive) {
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

    XAMP_LOG_D(logger_, "Start fetch cd information form music brainz.");

    http_client_.setUrl(QString::fromStdString(url));
    http_client_.get().then([this, disc_id](const auto& content) {
        auto [image_url, mb_disc_id_info] = parseMbDiscIdXml(content);

        mb_disc_id_info.disc_id = disc_id;
        mb_disc_id_info.tracks.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
        });

        emit fetchMbDiscInfoCompleted(mb_disc_id_info);

        XAMP_LOG_D(logger_, "Start fetch cd cover image.");

        http_client_.setUrl(QString::fromStdString(image_url));
        http_client_.get().then([this, disc_id](const auto& content) {
            const auto cover_url = parseCoverUrl(content);
            http_client_.setUrl(cover_url);
            http_client_.download().then([this, disc_id](const auto& content) mutable {
                QPixmap cover;
                if (cover.loadFromData(content)) {
                    const auto cover_id = qImageCache.addImage(cover);
                    XAMP_LOG_D(logger_, "Download cover image completed.");
                    emit fetchDiscCoverCompleted(QString::fromStdString(disc_id), cover_id);
                }
            });
        });
    });
}
#endif

void BackgroundService::onBlurImage(const QString& cover_id, const QPixmap& image, QSize size) {
    if (image.isNull()) {
        emit blurImage(QImage());
        return;
    }    
    emit blurImage(blur_image_cache_.GetOrAdd(cover_id, [&]() {
        return image_util::blurImage(thread_pool_, image, size);
        }));
}

void BackgroundService::onTranslation(const QString& keyword, const QString& from, const QString& to) {
    const auto url =
        qFormat("https://translate.google.com/translate_a/single?client=gtx&sl=%3&tl=%2&dt=t&q=%1")
        .arg(QString::fromStdString(QUrl::toPercentEncoding(keyword).toStdString()))
        .arg(to)
        .arg(from);
    http_client_.setUrl(url);
    http_client_.get().then([keyword, this](const auto& content) {
        if (content.isEmpty()) {
            return;
        }
        auto result = content;
        result = result.replace("[[[\""_str, ""_str);
        result = result.mid(0, result.indexOf(",\""_str) - 1);
        emit translationCompleted(keyword, result);
        });
}
