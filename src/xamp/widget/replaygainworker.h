//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <base/ithreadpool.h>
#include <widget/playlistentity.h>

using xamp::base::AlignPtr;
using xamp::base::IThreadPool;

struct ReplayGainResult final {
    double album_peak{0};
    double album_replay_gain{0};
    std::vector<PlayListEntity> music_id;
    std::vector<double> lufs;
    std::vector<double> track_peak;
    std::vector<double> track_replay_gain;
};

class ReplayGainWorker : public QObject {
    Q_OBJECT
public:
    ReplayGainWorker();

    void stopThreadPool();

    virtual ~ReplayGainWorker();

signals:
    void updateReplayGain(int music_id,
                    double album_rg_gain,
                    double album_peak,
                    double track_rg_gain,
                    double track_peak);

public Q_SLOT:
    void addEntities(bool force, const std::vector<PlayListEntity>& items);

private:
    bool is_stop_{false};
    AlignPtr<IThreadPool> pool_;
};