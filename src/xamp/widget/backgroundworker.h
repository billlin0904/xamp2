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
    double album_loudness{0};
    double album_peak{0};
    double album_gain{0};
    double album_peak_gain{0};
    Vector<double> track_loudness;
    Vector<double> track_peak;
    Vector<double> track_gain;
    Vector<double> track_peak_gain;
    Vector<PlayListEntity> music_id;
};

class BackgroundWorker : public QObject {
    Q_OBJECT
public:
    BackgroundWorker();

    void stopThreadPool();

    virtual ~BackgroundWorker();

signals:
    void updateReplayGain(int32_t playlistId,
        const PlayListEntity &entity,
        double track_loudness,
        double album_rg_gain,
        double album_peak,
        double track_rg_gain,
        double track_peak);

    void updateBlurImage(const QImage& image);

    void updateCdMetadata(const QString& disc_id, const ForwardList<TrackInfo>& track_infos);

    void updateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info);

    void updateDiscCover(const QString& disc_id, const QString& cover_id);

    void downloadPodcastCompleted(const ForwardList<TrackInfo>& track_infos, const QByteArray& cover_image_data);

public Q_SLOT:
    void onReadReplayGain(int32_t playlistId, const ForwardList<PlayListEntity>& entities);

    void onBlurImage(const QString &cover_id, const QImage& image);

    void onFetchCdInfo(const DriveInfo &drive);

    void onDownloadPodcast();

private:
    void lazyInit();

    bool is_stop_{false};
    mutable LruCache<QString, QImage> blur_img_cache_;
    AlignPtr<IThreadPoolExecutor> executor_;
    AlignPtr<IMetadataWriter> writer_;
    LoggerPtr logger_;
};
