//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QNetworkAccessManager>

#include <widget/widget_shared.h>
#include <widget/playlistentity.h>
#include <widget/driveinfo.h>
#include <widget/widget_shared_global.h>
#include <widget/util/mbdiscid_uiltis.h>

struct XAMP_WIDGET_SHARED_EXPORT ReplayGainResult final {
	double album_loudness{0};
	double album_peak{0};
	double album_gain{0};
	double album_peak_gain{0};
	Vector<double> track_loudness;
	Vector<double> track_peak;
	Vector<double> track_gain;
	Vector<double> track_peak_gain;
};

class XAMP_WIDGET_SHARED_EXPORT BackgroundWorker final : public QObject {
	Q_OBJECT

public:
	BackgroundWorker();

	~BackgroundWorker() override;

signals:
	void readFileStart();

	void readCompleted();

	void readFilePath(const QString& file_path);

	void readFileProgress(int32_t progress);

	void foundFileCount(size_t file_count);

	void readReplayGain(int32_t playlistId,
	                    const PlayListEntity& entity,
	                    double track_loudness,
	                    double album_rg_gain,
	                    double album_peak,
	                    double track_rg_gain,
	                    double track_peak);

	void blurImage(const QImage& image);

	void dominantColor(const QColor& color);

	void readCdTrackInfo(const QString& disc_id, const ForwardList<TrackInfo>& track_infos);

	void fetchMbDiscInfoCompleted(const MbDiscIdInfo& mb_disc_id_info);

	void fetchDiscCoverCompleted(const QString& disc_id, const QString& cover_id);

	void fetchLyricsCompleted(int32_t music_id, const QString& lyrics, const QString& trlyrics);

	void fetchArtistCompleted(const QString& artist, const QByteArray& image);

	void translationCompleted(const QString& keyword, const QString& result);

public Q_SLOT:
	void cancelRequested();

	void onReadReplayGain(int32_t playlistId, const QList<PlayListEntity>& entities);

	void onBlurImage(const QString& cover_id, const QPixmap& image, QSize size);

#if defined(Q_OS_WIN)
	void onFetchCdInfo(const DriveInfo& drive);
#endif

	void onSearchLyrics(int32_t music_id, const QString& title, const QString& artist);

	void onTranslation(const QString& keyword, const QString& from, const QString& to);

private:
	bool is_stop_{false};
	LruCache<QString, QImage> blur_image_cache_;
	LoggerPtr logger_;
	QNetworkAccessManager nam_;
};
