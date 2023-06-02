//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/widget_shared.h>
#include <widget/playlistentity.h>
#include <widget/driveinfo.h>
#include <widget/podcast_uiltis.h>
#include <widget/widget_shared_global.h>

class DatabaseFacade;

struct XAMP_WIDGET_SHARED_EXPORT ReplayGainResult final {
    double album_loudness{0};
    double album_peak{0};
    double album_gain{0};
    double album_peak_gain{0};
    Vector<double> track_loudness;
    Vector<double> track_peak;
    Vector<double> track_gain;
    Vector<double> track_peak_gain;
    Vector<PlayListEntity> play_list_entities;
};

class XAMP_WIDGET_SHARED_EXPORT BackgroundWorker : public QObject {
    Q_OBJECT
public:
    BackgroundWorker();

    void StopThreadPool();

    virtual ~BackgroundWorker() override;

signals:
    void ReadReplayGain(int32_t playlistId,
        const PlayListEntity &entity,
        double track_loudness,
        double album_rg_gain,
        double album_peak,
        double track_rg_gain,
        double track_peak);

    void BlurImage(const QImage& image);

    void DominantColor(const QColor &color);

    void OnReadCdTrackInfo(const QString& disc_id, const Vector<TrackInfo>& track_infos);

    void OnMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info);

    void OnDiscCover(const QString& disc_id, const QString& cover_id);

    void FetchPodcastCompleted(const Vector<TrackInfo>& track_infos, const QByteArray& cover_image_data);

    void FetchPodcastError(const QString& msg);    

    void SearchLyricsCompleted(int32_t music_id, const QString& lyrics, const QString& trlyrics);

    void SearchArtistCompleted(const QString& artist, const QByteArray& image);

    void TranslationCompleted(const QString& keyword, const QString& result);
public Q_SLOT:
	void OnFetchPodcast(int32_t playlist_id);

    void OnReadReplayGain(int32_t playlistId, const QList<PlayListEntity>& entities);

    void OnBlurImage(const QString &cover_id, const QPixmap& image, QSize size);

#if defined(Q_OS_WIN)
    void OnFetchCdInfo(const DriveInfo &drive);
#endif

    void OnSearchLyrics(int32_t music_id, const QString &title, const QString &artist);

    void OnGetArtist(const QString& artist);

    void OnTanslation(const QString& keyword, const QString& from, const QString& to);

private:
    bool is_stop_{false};    
    LoggerPtr logger_;
};
