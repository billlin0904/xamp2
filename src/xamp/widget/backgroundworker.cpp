#include <player/ebur128replaygain_scanner.h>

#include <base/logger.h>
#include <base/ithreadpool.h>
#include <base/math.h>
#include <metadata/api.h>
#include <metadata/imetadatawriter.h>
#include <widget/str_utilts.h>
#include <widget/stackblur.h>
#include <widget/ui_utilts.h>
#include <widget/read_utiltis.h>
#include <widget/appsettings.h>
#include <widget/colorthief.h>
#include <widget/backgroundworker.h>

BackgroundWorker::BackgroundWorker() {
    pool_ = MakeThreadPool(kBackgroundThreadPoolLoggerName,
        std::thread::hardware_concurrency(),
        kDefaultAffinityCpuCore, 
        ThreadPriority::BACKGROUND);
}

BackgroundWorker::~BackgroundWorker() = default;

void BackgroundWorker::stopThreadPool() {
    is_stop_ = true;
    pool_->Stop();
}

void BackgroundWorker::blurImage(const QString& cover_id, const QImage& image) {
    auto palettes = GetPalette(image);

    if (auto *cache_image = blur_img_cache_.Find(cover_id)) {
        XAMP_LOG_DEBUG("Found blur image in cache!");
        emit updateBlurImage(cache_image->copy());
    } else {
        auto temp = image.copy();
        XAMP_LOG_DEBUG("Blur image start");
        Stopwatch sw;
        sw.Reset();
        Stackblur blur(*pool_, temp, 50);
        emit updateBlurImage(temp.copy());
        blur_img_cache_.AddOrUpdate(cover_id, std::move(temp));
        XAMP_LOG_DEBUG("Blur image end :{} secs", Round(sw.ElapsedSeconds()), 2);
    }
}

void BackgroundWorker::readReplayGain(bool force, const std::vector<PlayListEntity>& items) {
    XAMP_LOG_DEBUG("Start read replay gain count:{}", items.size());

    std::vector<std::shared_future<PlayListEntity>> replay_gain_tasks;

    const auto target_gain = AppSettings::getValue(kAppSettingReplayGainTargetGain).toDouble();
    const auto scan_mode = AppSettings::getAsEnum<ReplayGainScanMode>(kAppSettingReplayGainScanMode);

    std::vector<AlignPtr<Ebur128ReplayGainScanner>> scanners;
    FastMutex mutex;

    scanners.reserve(items.size());
    replay_gain_tasks.reserve(items.size());

    for (const auto &item : items) {
        replay_gain_tasks.push_back(pool_->Spawn([&scanners, scan_mode, item, &mutex, this](auto ) {
            auto progress = [scan_mode](auto p) {
                if (scan_mode == ReplayGainScanMode::RG_SCAN_MODE_FAST && p > 50) {
                    return false;
                }
                return true;
            };

            AlignPtr<Ebur128ReplayGainScanner> scanner;
            auto prepare = [&scanner](auto const& input_format) mutable {
                scanner = MakeAlign<Ebur128ReplayGainScanner>(input_format.GetSampleRate());
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
                XAMP_ASSERT(scanner != nullptr);
                scanners.push_back(std::move(scanner));
            }
            catch (std::exception const& e) {
                XAMP_LOG_DEBUG("{}", e.what());
            }
            return item;
        }));
    }

    ReplayGainResult replay_gain;

    for (auto const& task : replay_gain_tasks) {
        try {
            replay_gain.music_id.push_back(task.get());
        }
        catch (std::exception const& e) {
            XAMP_LOG_DEBUG("ReplayGain got exception! {}", e.what());
        }
    }

    if (scanners.empty()) {
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
        const auto track_peak = scanner->GetSamplePeak();
        replay_gain.album_peak = (std::min)(track_peak, replay_gain.album_peak);
        replay_gain.track_peak.push_back(track_peak);
        const auto track_lufs = scanner->GetLoudness();
        replay_gain.lufs.push_back(track_lufs);
        replay_gain.track_replay_gain.push_back(Ebur128ReplayGainScanner::GetEbur128Gain(track_lufs, target_gain));
    }

    using namespace xamp::metadata;
    const auto writer = MakeMetadataWriter();

    for (size_t i = 0; i < replay_gain_tasks.size(); ++i) {
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
