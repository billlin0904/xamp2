#include <vector>

#include <player/loudness_scanner.h>

#include <base/logger.h>
#include <base/ithreadpool.h>
#include <metadata/api.h>
#include <metadata/imetadatawriter.h>

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
    std::vector<std::shared_future<PlayListEntity>> replay_gain_tasks;

    const auto target_gain = AppSettings::getValue(kAppSettingReplayGainTargetGain).toDouble();
    const auto scan_mode = AppSettings::getAsEnum<ReplayGainScanMode>(kAppSettingReplayGainScanMode);

    std::vector<AlignPtr<LoudnessScanner>> scanners;
    FastMutex mutex;

    scanners.reserve(items.size());

    replay_gain_tasks.reserve(items.size());

    for (const auto &item : items) {
        replay_gain_tasks.emplace_back(pool_->Spawn([&scanners, scan_mode, item, &mutex, this](auto ) {
	        try {
		        AlignPtr<LoudnessScanner> scanner;
		        read_utiltis::readAll(item.file_path.toStdWString(),
		                              [scan_mode](auto progress) {
                                          if (scan_mode == ReplayGainScanMode::RG_SCAN_MODE_FAST && progress > 50) {
                                              return false;
                                          }
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
