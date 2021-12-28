#include <vector>

#include <player/loudness_scanner.h>

#include <base/logger.h>
#include <base/ithreadpool.h>
#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <widget/read_utiltis.h>
#include <widget/appsettings.h>
#include <widget/replaygainworker.h>

ReplayGainWorker::ReplayGainWorker() {
    pool_ = MakeThreadPool(kReplayGainThreadPoolLoggerName);
}

ReplayGainWorker::~ReplayGainWorker() {    
    pool_->Stop();
}

void ReplayGainWorker::stopThreadPool() {
    is_stop_ = true;
}

void ReplayGainWorker::addEntities(bool force, const std::vector<PlayListEntity>& items) {
    std::vector<std::shared_future<int32_t>> replay_gain_tasks;

    const auto target_gain = AppSettings::getValue(kAppSettingReplayGainTargetGain).toDouble();

    std::vector<AlignPtr<LoudnessScanner>> scanners;
    FastMutex mutex;

    scanners.reserve(items.size());

    replay_gain_tasks.reserve(items.size());

    for (const auto &item : items) {
        replay_gain_tasks.emplace_back(pool_->Spawn([&scanners, force, item, &mutex, this](auto ) {
            AlignPtr<LoudnessScanner> scanner;
            if (!force && item.album_replay_gain != 0.0) {
                return item.music_id;
            }

            try {
                read_utiltis::readAll(item.file_path.toStdWString(),
                    [](auto progress) {
                        return true;
                    },
                    [&scanner](AudioFormat const& input_format) mutable {
                        scanner = MakeAlign<LoudnessScanner>(input_format.GetSampleRate());
                    }, [&scanner, this](auto const* samples, auto sample_size) {
                        if (is_stop_) {
                            return;
                        }
                        scanner->Process(samples, sample_size);
                    });

                std::lock_guard<FastMutex> guard{ mutex };
                scanners.push_back(std::move(scanner));
            }
            catch (std::exception const& e) {
                XAMP_LOG_DEBUG("{}", e.what());
            }
            return item.music_id;
        }));
    }

    ReplayGain result;

    try {
        for (auto const& task : replay_gain_tasks) {
            result.music_id.push_back(task.get());
        }
    }
    catch (...) {
        return;
    }

    if (scanners.empty()) {
        XAMP_LOG_DEBUG("ReplayGain no more work item!");
        return;
    }

    result.track_peak.reserve(items.size());
    result.lufs.reserve(items.size());
    result.track_replay_gain.reserve(items.size());

    result.album_replay_gain = LoudnessScanner::GetEbur128Gain(
        LoudnessScanner::GetMultipleEbur128Gain(scanners),
        target_gain);

    result.album_peak = 100.0;
    for (auto const &scanner : scanners) {
        if (!scanner) {
            continue;
        }
        result.album_peak = (std::min)(scanner->GetSamplePeak(), result.album_peak);
        result.track_peak.push_back(scanner->GetSamplePeak());
        result.lufs.push_back(scanner->GetLoudness());
        result.track_replay_gain.push_back(LoudnessScanner::GetEbur128Gain(scanner->GetLoudness(), target_gain));
    }

    for (auto i = 0; i < replay_gain_tasks.size(); ++i) {
        XAMP_LOG_DEBUG("Music id: {} LUFS : {} ", result.music_id[i], result.lufs[i]);
        emit updateLUFS(result.music_id[i],
                        result.album_replay_gain,
                        result.album_peak,
                        result.track_replay_gain[i],
                        result.track_peak[i]
                        );
    }
}
