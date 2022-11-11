//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <widget/widget_shared.h>
#include <widget/playlistentity.h>
#include <widget/driveinfo.h>
#include <widget/podcast_uiltis.h>

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
    void updateReplayGain(int32_t music_id,
                    double album_rg_gain,
                    double album_peak,
                    double track_rg_gain,
                    double track_peak);

    void updateBlurImage(const QImage& image);

    void updateCdMetadata(const QString& disc_id, const ForwardList<Metadata>& metadatas);

    void updateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info);

    void updateDiscCover(const QString& disc_id, const QString& cover_id);

public Q_SLOT:
    void onReadReplayGain(bool force, const Vector<PlayListEntity>& items);

    void onBlurImage(const QString &cover_id, const QImage& image);

    void onFetchCdInfo(const DriveInfo &drive);

private:
    bool is_stop_{false};
    mutable LruCache<QString, QImage> blur_img_cache_;
    AlignPtr<IThreadPoolExecutor> pool_;
    AlignPtr<IMetadataWriter> writer_;
};
