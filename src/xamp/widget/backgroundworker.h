//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <base/align_ptr.h>
#include <base/ithreadpool.h>
#include <base/lrucache.h>
#include <metadata/api.h>
#include <widget/playlistentity.h>

using xamp::base::AlignPtr;
using xamp::base::IThreadPool;
using xamp::base::LruCache;
using xamp::base::Vector;
using xamp::metadata::IMetadataWriter;

struct ReplayGainResult final {
    double album_peak{0};
    double album_replay_gain{0};
    Vector<PlayListEntity> music_id;
    Vector<double> lufs;
    Vector<double> track_peak;
    Vector<double> track_replay_gain;
};

class BackgroundWorker : public QObject {
    Q_OBJECT
public:
    BackgroundWorker();

    void stopThreadPool();

    virtual ~BackgroundWorker();

signals:
    void updateReplayGain(int music_id,
                    double album_rg_gain,
                    double album_peak,
                    double track_rg_gain,
                    double track_peak);

    void updateBlurImage(const QImage& image);

public Q_SLOT:
    void readReplayGain(bool force, const std::vector<PlayListEntity>& items);

    void blurImage(const QString &cover_id, const QImage& image);

private:
    bool is_stop_{false};
    mutable LruCache<QString, QImage> blur_img_cache_;
    AlignPtr<IThreadPool> pool_;
    AlignPtr<IMetadataWriter> writer_;
};
