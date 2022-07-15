#include <player/ebur128replaygain_scanner.h>

#include <widget/widget_shared.h>
#include <base/logger_impl.h>

#include <widget/str_utilts.h>
#include <widget/appsettingnames.h>
#include <widget/stackblur.h>
#include <widget/ui_utilts.h>
#include <widget/read_utiltis.h>
#include <widget/appsettings.h>
#include <widget/backgroundworker.h>

struct PlayListRGResult {
    PlayListRGResult(PlayListEntity item, std::optional<Ebur128ReplayGainScanner> scanner)
        : item(std::move(item))
        , scanner(std::move(scanner)) {
    }
    PlayListEntity item;
    std::optional<Ebur128ReplayGainScanner> scanner;
};

using xamp::metadata::MakeMetadataWriter;

BackgroundWorker::BackgroundWorker()
	: blur_img_cache_(8) {
    pool_ = MakeThreadPool(kBackgroundThreadPoolLoggerName,
        TaskSchedulerPolicy::LEAST_LOAD_POLICY,
        TaskStealPolicy::CONTINUATION_STEALING_POLICY,
        ThreadPriority::BACKGROUND);
    writer_ = MakeMetadataWriter();
}

BackgroundWorker::~BackgroundWorker() = default;

void BackgroundWorker::stopThreadPool() {
    is_stop_ = true;
    pool_->Stop();
}

void BackgroundWorker::blurImage(const QString& cover_id, const QImage& image) {
    if (auto *cache_image = blur_img_cache_.Find(cover_id)) {
        XAMP_LOG_DEBUG("Found blur image in cache!");
        emit updateBlurImage(cache_image->copy());
        return;
    }

    if (!AppSettings::getValueAsBool(kEnableBlurCover)) {
        emit updateBlurImage(QImage());
        return;
    }
    auto temp = image.copy();
    Stackblur blur(*pool_, temp, 50);
    emit updateBlurImage(temp.copy());
    blur_img_cache_.AddOrUpdate(cover_id, std::move(temp));
}

void BackgroundWorker::readReplayGain(bool, const Vector<PlayListEntity>& items) {
    XAMP_LOG_DEBUG("Start read replay gain count:{}", items.size());

    Vector<SharedFuture<AlignPtr<PlayListRGResult>>> replay_gain_tasks;

    const auto target_gain = AppSettings::getValue(kAppSettingReplayGainTargetGain).toDouble();
    const auto scan_mode = AppSettings::getAsEnum<ReplayGainScanMode>(kAppSettingReplayGainScanMode);

    replay_gain_tasks.reserve(items.size());

    Stopwatch wait_time_sw;
    double process_time = 0;

    for (const auto &item : items) {
        replay_gain_tasks.push_back(pool_->Spawn([scan_mode, item, &process_time, this](auto ) ->AlignPtr<PlayListRGResult> {
            auto progress = [scan_mode](auto p) {
                if (scan_mode == ReplayGainScanMode::RG_SCAN_MODE_FAST && p > 50) {
                    return false;
                }
                return true;
            };

            std::optional<Ebur128ReplayGainScanner> scanner;
            auto prepare = [&scanner](auto const& input_format) mutable {
                scanner = Ebur128ReplayGainScanner(input_format.GetSampleRate());
            };

            auto dps_process = [&scanner, this](auto const* samples, auto sample_size) {
                if (is_stop_) {
                    return;
                }
                scanner.value().Process(samples, sample_size);
            };

            Stopwatch sw;
            read_utiltis::readAll(item.file_path.toStdWString(), progress, prepare, dps_process);
            process_time = sw.ElapsedSeconds();
            return MakeAlign<PlayListRGResult>(item, std::move(scanner));
        }));
    }

    Vector<Ebur128ReplayGainScanner> scanners;
    ReplayGainResult replay_gain;

    for (auto & task : replay_gain_tasks) {
        try {
            auto& result = task.get();
            if (!result) {
                continue;
            }
            XAMP_LOG_DEBUG("Process replaygain process:{:.2f} secs wait :{:.2f} secs",
                process_time,
                wait_time_sw.ElapsedSeconds());
            wait_time_sw.Reset();
            replay_gain.music_id.push_back(result->item);
            if (result->scanner.has_value()) {
                scanners.push_back(std::move(result->scanner.value()));
            }
        }
        catch (std::exception const& e) {
            XAMP_LOG_DEBUG("ReplayGain got exception! {}", e.what());
        }
    }

    if (scanners.empty() || replay_gain_tasks.size() != scanners.size()) {
        XAMP_LOG_DEBUG("ReplayGain no more work item!");
        return;
    }

    replay_gain.track_peak.reserve(items.size());
    replay_gain.lufs.reserve(items.size());
    replay_gain.track_replay_gain.reserve(items.size());

    replay_gain.album_replay_gain = Ebur128ReplayGainScanner::GetEbur128Gain(
        Ebur128ReplayGainScanner::GetMultipleLoudness(scanners),
        target_gain);
    
    replay_gain.album_peak = 100.0;
    for (auto const &scanner : scanners) {
        const auto track_peak = scanner.GetSamplePeak();
        replay_gain.album_peak = (std::min)(track_peak, replay_gain.album_peak);
        replay_gain.track_peak.push_back(track_peak);
        const auto track_lufs = scanner.GetLoudness();
        replay_gain.lufs.push_back(track_lufs);
        replay_gain.track_replay_gain.push_back(
            Ebur128ReplayGainScanner::GetEbur128Gain(track_lufs, target_gain));
    }

    for (size_t i = 0; i < replay_gain_tasks.size(); ++i) {
	    ReplayGain rg;
        rg.album_gain = replay_gain.album_replay_gain;
        rg.track_gain = replay_gain.track_replay_gain[i];
        rg.album_peak = replay_gain.album_peak;
        rg.track_peak = replay_gain.track_peak[i];
        rg.ref_loudness = target_gain;
        writer_->WriteReplayGain(replay_gain.music_id[i].file_path.toStdWString(), rg);
        emit updateReplayGain(replay_gain.music_id[i].music_id,
                        replay_gain.album_replay_gain,
                        replay_gain.album_peak,
                        replay_gain.track_replay_gain[i],
                        replay_gain.track_peak[i]
                        );
    }
}
