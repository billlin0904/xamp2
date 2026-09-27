#include <widget/worker/backgroundservice.h>

#include <widget/util/ui_util.h>
#include <widget/util/str_util.h>
#include <widget/util/image_util.h>
#include <widget/database.h>
#include <widget/databasefacade.h>
#include <widget/util/mbdiscid_util.h>
#include <widget/appsettings.h>
#include <widget/widget_shared.h>
#include <widget/imagecache.h>
#include <widget/util/json_util.h>
#include <widget/util/tag_util.h>

#include <widget/appsettingnames.h>

#include <stream/filestream.h>
#include <base/logger.h>
#include <base/threadpoolbuilder.h>

#if defined(Q_OS_WIN)
#include <stream/mbdiscid.h>
#endif

#include <QDir>
#include <QFuture>
#include <QJsonValueRef>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

namespace {
    XAMP_DECLARE_LOG_NAME(BackgroundService);
}

BackgroundService::BackgroundService()
    : nam_(this)
	, http_client_(&nam_, QString(), this) {
    logger_ = XAMP_LOG_CREATE_LOGGER(BackgroundService);
    thread_pool_ = ThreadPoolBuilder::makeBackgroundThreadPool();
	lyrics_sources_.push_back(makeNeteaseLyricsSource(&nam_, this));
	lyrics_sources_.push_back(makeQQMusicLyricsSource(&nam_, this));
	lyrics_sources_.push_back(makeKugouLyricsSource(&nam_, this));
}

BackgroundService::~BackgroundService() = default;

void BackgroundService::cancelAllJob() {
    stop_source_.request_stop();
}

std::tuple<std::shared_ptr<FastIOStream>, Path> 
BackgroundService::makeUniqueFile(const EncodeJob &job,
    const QString& dir_name) {
    std::shared_ptr<FastIOStream> file_writer;
    Path output_path;

    auto file_name = getValidFileName(job.file_.file_name);

    // Ensure file name is unique.
    constexpr auto kMaxRetryTestUniqueFileName = 128;
    auto i = 0;
    for (; i < kMaxRetryTestUniqueFileName; ++i) {
        try {
            QString unique_save_file_name;
            switch (job.type) {
            case EncodeType::ENCODE_AAC:
            case EncodeType::ENCODE_ALAC:
                unique_save_file_name = uniqueFileName(
                    QDir(dir_name),
                    file_name + ".m4a"_str);
                break;
            case EncodeType::ENCODE_PCM:
                unique_save_file_name = uniqueFileName(
                    QDir(dir_name),
                    file_name + ".wav"_str);
                break;
            default:
                return std::make_tuple(nullptr, "");
            }

            auto save_file_name = dir_name
                + "/"_str
                + unique_save_file_name;
            output_path = save_file_name.toStdWString();
            file_writer = std::make_shared<FastIOStream>(output_path, FastIOStream::Mode::ReadWrite);
            break;
        }
        catch (const Exception& e) {
            XAMP_LOG_ERROR(e.getErrorMessage());
        }
    }
    if (i == kMaxRetryTestUniqueFileName) {
        XAMP_LOG_ERROR("Failed to create unique file name.");
        return std::make_tuple(nullptr, "");
    }
    return std::make_tuple(file_writer, output_path);
}

void BackgroundService::sequenceEncode(const QString& dir_name, QList<EncodeJob> jobs) {
    Q_FOREACH(auto job, jobs) {
		executeEncodeJob(dir_name, job);
    }
}

void BackgroundService::executeEncodeJob(const QString& dir_name, const EncodeJob & job) {
    auto [file_writer, output_path] = makeUniqueFile(
        job,
        dir_name);

    if (file_writer == nullptr || output_path.empty()) {
        emit jobError(job.job_id, tr("executeEncodeJob error"));
        return;
    }

    Path input_path(job.file_.file_path.toStdWString());
    auto stop_token = stop_source_.get_token();

    try {
        auto encoder = StreamFactory::makeFileEncoder();

        Property config;
        config.create(FileEncoderConfig::kInputFilePath,
            input_path);
        config.create(FileEncoderConfig::kOutputFilePath,
            output_path);
        config.create(FileEncoderConfig::kCodecId,
            job.codec_id.toStdString());
        config.create(FileEncoderConfig::kBitRate,
            job.bit_rate);

        encoder->start(config, file_writer);
        encoder->encode([job, &stop_token, this](auto progress) {
            if (stop_token.stop_requested()) {
                return false;
            }
            if (progress % 10 == 0) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(100));
                emit updateJobProgress(job.job_id, progress);
            }            
            return true;
            });
        file_writer.reset();
        encoder.reset();

        if (stop_token.stop_requested()) {
            Fs::remove(output_path);
            XAMP_LOG_DEBUG("executeEncodeJob job canceled.");
            emit jobError(job.job_id, tr("Canceled"));
			return;
        }

        auto writer = makeMetadataWriter();
        writer->open(output_path);
        writer->writeArtist(job.file_.artist.toStdWString());
        writer->writeTitle(job.file_.title.toStdWString());
        writer->writeAlbum(job.file_.album.toStdWString());
        writer->writeComment(job.file_.comment.toStdWString());
        writer->writeGenre(job.file_.genre.toStdWString());
        writer->writeTrack(job.file_.track);
        writer->writeYear(job.file_.year);

        auto reader = makeMetadataReader();
        reader->open(input_path);
        auto cover = tag_util::readEmbeddedCover(*reader);
        if (!cover.isNull()) {
            tag_util::writeEmbeddedCover(*writer, cover);
        }

        emit updateJobProgress(job.job_id, 100);
    }
    catch (const Exception& e) {
        XAMP_LOG_ERROR(e.getStackTrace());
        emit jobError(job.job_id, tr("executeEncodeJob error"));
    }
    catch (const std::exception& e) {
        XAMP_LOG_ERROR(e.what());
        emit jobError(job.job_id, tr("executeEncodeJob error"));
    }
}

QCoro::Task<std::optional<QByteArray>> BackgroundService::tryFetch(const QString& tag, const QString& release_id, size_t size) {
    const auto url = (size > 0)
        ? qFormat("https://coverartarchive.org/%1/%2/front-%3").arg(tag).arg(release_id).arg(size)
        : qFormat("https://coverartarchive.org/%1/%2/front").arg(tag).arg(release_id);
    http_client_.setUrl(url);
    http_client_.setHeader("Accept"_str, "image/*"_str);

    auto img = co_await http_client_.download();
    if (!img.isEmpty())
        co_return img;
    co_return std::nullopt;
}

QCoro::Task<std::optional<QByteArray>> BackgroundService::fetchCoverArtByUrl(const QString& tag, const QString& release_id, size_t prefer_size) {
    std::optional<QByteArray> b;
    if (prefer_size > 0) {
        auto result = co_await tryFetch(tag, release_id, prefer_size);
        if (result.has_value()) {
            b = result.value();
        } else {
            result = co_await tryFetch(tag, release_id, 500);
            if (result.has_value()) {
                b = result.value();
            }
        }
    } else {
        auto result = co_await tryFetch(tag, release_id, -1);
        if (result.has_value()) {
            b = result.value();
        }
    }
    if (!b.has_value()) {
        XAMP_LOG_DEBUG("Not found cover art.");
        co_return std::nullopt;
	}
	co_return b;
}

void BackgroundService::parallelEncode(const QString& dir_name, QList<EncodeJob> jobs) {
    auto stop_token = stop_source_.get_token();
    Executor::parallelFor(thread_pool_, jobs,
        [this, dir_name](const EncodeJob& job) {
        executeEncodeJob(dir_name, job);
        }, stop_token);
}

void BackgroundService::onAddJobs(const QString& dir_name, const QList<EncodeJob>& jobs) {
    stop_source_ = std::stop_source();

    QList<EncodeJob> parallel_jobs;
	Q_FOREACH(auto job, jobs) {
		if (job.file_.disc_id.isEmpty()) {
			parallel_jobs.push_back(job);
		}
		else {
			sequenceEncode(dir_name, { job });
		}
    }
    
    parallelEncode(dir_name, parallel_jobs);
    //sequenceEncode(dir_name, jobs);
}

void BackgroundService::cancelRequested() {
    is_stop_ = true;
}

QCoro::Task<> BackgroundService::searchLyrics(const PlayListEntity& keyword) {
    auto temp = keyword.cleanup();
	const auto request_title = temp.title;
	const auto request_artist = temp.artist;
    QList<SearchLyricsResult> fallback_results;
	QList<SearchLyricsResult> all_results;

	XAMP_LOG_DEBUG("Search lyrics start title:'{}' artist:'{}' album:'{}'.",
		temp.title.toStdString(),
		temp.artist.toStdString(),
		temp.album.toStdString());

	for (auto& source : lyrics_sources_) {
		try {
			XAMP_LOG_DEBUG("Search lyrics source:{} begin.", source->name().toStdString());
			auto candidates = co_await source->search(temp);
			XAMP_LOG_DEBUG("Search lyrics source:{} candidates:{}.",
				source->name().toStdString(),
				candidates.size());
			all_results.reserve(all_results.size() + candidates.size());
			

			for (const auto& candidate : candidates) {
				auto result = co_await source->lookup(candidate);
				if (result.parsers.empty()) {
					continue;
				}
				auto itr = std::find_if(result.parsers.begin(), result.parsers.end(),
					[](const auto& parser) {
					return parser.parser->isKaraoke();
					});
				if (itr != result.parsers.end()) {
                    result.request_title = request_title;
                    result.request_artist = request_artist;
                    all_results.push_back(std::move(result));
                    break;
				}                
				result.request_title = request_title;
				result.request_artist = request_artist;
                fallback_results.push_back(std::move(result));				
			}
			XAMP_LOG_DEBUG("Search lyrics source:{} accumulated_results:{}.",
				source->name().toStdString(),
				all_results.size());
		}
		catch (const Exception& e) {
			XAMP_LOG_ERROR(e.getErrorMessage());
		}
		catch (const std::exception& e) {
			XAMP_LOG_ERROR(e.what());
		}
	}

	if (!all_results.empty()) {		
        emit fetchLyricsCompleted(all_results);
		XAMP_LOG_DEBUG("Search lyrics completed results:{}.", all_results.size());		
	}
	else {
        emit fetchLyricsCompleted(fallback_results);
		XAMP_LOG_DEBUG("Search lyrics completed with no results.");
	}

    co_return;
}

void BackgroundService::onSearchLyrics(const PlayListEntity& keyword) {
    auto temp = keyword.cleanup();
    searchLyrics(temp).then([]() {});
}

#if defined(Q_OS_WIN)
void BackgroundService::onFetchCdInfo(const DriveInfo& drive) {
    MBDiscId mbdisc_id;
    std::string disc_id;
    std::string url;

    try {
        disc_id = mbdisc_id.getDiscId(drive.drive_path.toStdString());
        url = mbdisc_id.getDiscIdLookupUrl(drive.drive_path.toStdString());
    } catch (const Exception &e) {
        XAMP_LOG_DEBUG(e.getErrorMessage());
        return;
    }

    XAMP_LOG_D(logger_, "start fetch cd information form musicbrainz.");

    http_client_.setUrl(QString::fromStdString(url));
    http_client_.get().then([this, drive, disc_id](const auto& content) {
        auto [image_url, mb_disc_id_info] = parseMbDiscIdXml(content);

        std::forward_list<TrackInfo> track_infos;
        const auto cd = openCD(drive.driver_letter);
        cd->setMaxSpeed();
        const auto tracks = cd->getTotalTracks();

        auto track_id = 0;
        for (const auto& track : tracks) {
            TrackInfo track_info;
            auto reader = makeMetadataReader();
            reader->open(track);
            auto track_info_opt = reader->extract();
            if (track_info_opt.has_value()) {
				track_info = track_info_opt.value();
            }
            track_info.title = mb_disc_id_info.tracks[track_id].title;
            track_info.file_path = track;
            track_info.duration = cd->getDuration(track_id++);
            track_info.album = mb_disc_id_info.album;
            track_info.sample_rate = AudioFormat::k16BitPCM441Khz.getSampleRate();
            track_info.disc_id = disc_id;
            track_info.track = track_id;
            track_infos.push_front(track_info);
        }

        track_infos.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });

        emit readCdTrackInfo(QString::fromStdString(disc_id), track_infos);

        mb_disc_id_info.disc_id = disc_id;
        std::sort(mb_disc_id_info.tracks.begin(), mb_disc_id_info.tracks.end(), 
            [](const auto& first, const auto& last) {
            return first.track < last.track;
            });

        emit fetchMbDiscInfoCompleted(mb_disc_id_info);

        XAMP_LOG_D(logger_, "start fetch cd cover image.");

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

void BackgroundService::onBlurImage(const QString& cover_id,
    const QPixmap& image, 
    QSize size) {
    if (image.isNull()) {
        emit blurImage(QImage());
        return;
    }    
    emit blurImage(blur_image_cache_.getOrAdd(cover_id, [&]() {
        return image_util::blurImage(thread_pool_, image, size);
        }));
}
