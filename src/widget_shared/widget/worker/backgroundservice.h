//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stop_token>

#include <QObject>
#include <base/object_pool.h>
#include <base/threadpoolexecutor.h>

#include <widget/widget_shared.h>
#include <widget/playlistentity.h>
#include <widget/driveinfo.h>
#include <widget/widget_shared_global.h>
#include <widget/util/mbdiscid_util.h>
#include <widget/httpx.h>
#include <widget/encodejobwidget.h>

Q_DECLARE_METATYPE(ReplayGain);

class XAMP_WIDGET_SHARED_EXPORT BackgroundService final : public QObject {
	Q_OBJECT

public:
	static constexpr size_t kBufferPoolSize = 256;

	BackgroundService();

	~BackgroundService() override;

signals:
	void blurImage(const QImage& image);

	void readCdTrackInfo(const QString& disc_id, const ForwardList<TrackInfo>& track_infos);

    void fetchDiscCoverCompleted(const QString& disc_id, const QString& cover_id);

	void fetchLyricsCompleted(int32_t music_id, const QString& lyrics, const QString& trlyrics);

	void translationCompleted(const QString& keyword, const QString& result);

	void fetchMbDiscInfoCompleted(const MbDiscIdInfo &info);

	void updateJobProgress(const QString& job_id, int new_progress);

	void jobError(const QString& job_id, const QString &message);

	void readAudioData(const std::vector<float>& interleaved);

	void readAudioDataCompleted();

public Q_SLOT:
	void cancelAllJob();

	void onAddJobs(const QString& dir_name, QList<EncodeJob> jobs);

	void cancelRequested();

	void onBlurImage(const QString& cover_id, const QPixmap& image, QSize size);

#if defined(Q_OS_WIN)
	void onFetchCdInfo(const DriveInfo& drive);
#endif

	void onSearchLyrics(int32_t music_id, const QString& title, const QString& artist);

	void onTranslation(const QString& keyword, const QString& from, const QString& to);

	void onReadWaveformAudioData(size_t frame_per_peek, const Path & file_path);

private:
	bool is_stop_{false};
	LruCache<QString, QImage> blur_image_cache_;
	LoggerPtr logger_;
	std::stop_source stop_source_;
	QNetworkAccessManager nam_;
	http::HttpClient http_client_;
	std::shared_ptr<ObjectPool<QByteArray>> buffer_pool_;
	std::shared_ptr<IThreadPoolExecutor> thread_pool_;
};
