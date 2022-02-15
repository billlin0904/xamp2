#include <player/loudness_scanner.h>

#include <base/logger.h>
#include <base/ithreadpool.h>
#include <metadata/api.h>
#include <metadata/imetadatawriter.h>

#include <widget/str_utilts.h>
#include <widget/stackblur.h>
#include <widget/ui_utilts.h>
#include <widget/read_utiltis.h>
#include <widget/appsettings.h>
#include <widget/backgroundworker.h>

BackgroundWorker::BackgroundWorker() {
    pool_ = MakeThreadPool(kBackgroundThreadPoolLoggerName);
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
    } else {
        auto temp = image.copy();
        XAMP_LOG_DEBUG("Blur image start");
        Stopwatch sw;
        sw.Reset();
        Stackblur blur(*pool_, temp, 50);
        XAMP_LOG_DEBUG("Blur image end :{} secs", sw.ElapsedSeconds());
        blur_img_cache_.AddOrUpdate(cover_id, temp);
        emit updateBlurImage(temp);
    }
}

void BackgroundWorker::readReplayGain(bool force, const std::vector<PlayListEntity>& items) {
    std::vector<std::shared_future<PlayListEntity>> replay_gain_tasks;

    const auto target_gain = AppSettings::getValue(kAppSettingReplayGainTargetGain).toDouble();
    const auto scan_mode = AppSettings::getAsEnum<ReplayGainScanMode>(kAppSettingReplayGainScanMode);

    std::vector<AlignPtr<LoudnessScanner>> scanners;
    FastMutex mutex;

    scanners.reserve(items.size());

    replay_gain_tasks.reserve(items.size());

    for (const auto &item : items) {
        replay_gain_tasks.emplace_back(pool_->Spawn([&scanners, scan_mode, item, &mutex, this](auto ) {
            auto progress = [scan_mode](auto p) {
                if (scan_mode == ReplayGainScanMode::RG_SCAN_MODE_FAST && p > 50) {
                    return false;
                }
                return true;
            };

            AlignPtr<LoudnessScanner> scanner;
            auto prepare = [&scanner](AudioFormat const& input_format) mutable {
                scanner = MakeAlign<LoudnessScanner>(input_format.GetSampleRate());
            };

            auto dps_process = [&scanner, this](auto const* samples, auto sample_size) {
                if (is_stop_) {
                    return;
                }
                scanner->Process(samples, sample_size);
            };

	        try {	        
		        read_utiltis::readAll(item.file_path.toStdWString(), progress, prepare, dps_process);
                std::lock_guard<FastMutex> guard{ mutex };
                scanners.push_back(std::move(scanner));
            }
            catch (std::exception const& e) {
                XAMP_LOG_DEBUG("{}", e.what());
            }
            return item;
        }));
    }

    ReplayGainResult replay_gain;

    try {
        for (auto const& task : replay_gain_tasks) {
            replay_gain.music_id.push_back(task.get());
        }
    }
    catch (...) {
        return;
    }

    if (scanners.empty()) {
        XAMP_LOG_DEBUG("ReplayGain no more work item!");
        return;
    }

    replay_gain.track_peak.reserve(items.size());
    replay_gain.lufs.reserve(items.size());
    replay_gain.track_replay_gain.reserve(items.size());

    replay_gain.album_replay_gain = LoudnessScanner::GetEbur128Gain(
        LoudnessScanner::GetMultipleEbur128Gain(scanners),
        target_gain);

    replay_gain.album_peak = 100.0;
    for (auto const &scanner : scanners) {
        const auto track_peak = scanner->GetSamplePeak();
        replay_gain.album_peak = (std::min)(track_peak, replay_gain.album_peak);
        replay_gain.track_peak.push_back(track_peak);
        const auto track_lufs = scanner->GetLoudness();
        replay_gain.lufs.push_back(track_lufs);
        replay_gain.track_replay_gain.push_back(LoudnessScanner::GetEbur128Gain(track_lufs, target_gain));
    }

    using namespace xamp::metadata;
    auto writer = MakeMetadataWriter();

    for (auto i = 0; i < replay_gain_tasks.size(); ++i) {
	    ReplayGain rg;
        rg.album_gain = replay_gain.album_replay_gain;
        rg.track_gain = replay_gain.track_replay_gain[i];
        rg.album_peak = replay_gain.album_peak;
        rg.track_peak = replay_gain.track_peak[i];
        rg.ref_loudness = target_gain;
        writer->WriteReplayGain(replay_gain.music_id[i].file_path.toStdWString(), rg);
        emit updateReplayGain(replay_gain.music_id[i].music_id,
                        replay_gain.album_replay_gain,
                        replay_gain.album_peak,
                        replay_gain.track_replay_gain[i],
                        replay_gain.track_peak[i]
                        );
    }
}
