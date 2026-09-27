//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <stop_token>
#include <vector>

#include <QObject>
#include <base/threadpool.h>
#include <base/lrucache.h>

#include <widget/widget_shared.h>
#include <widget/driveinfo.h>
#include <widget/widget_shared_global.h>
#include <widget/util/mbdiscid_util.h>
#include <widget/httpx.h>
#include <widget/encodejobwidget.h>
#include <widget/worker/lyrics_source.h>

Q_DECLARE_METATYPE(ReplayGain);

class XAMP_WIDGET_SHARED_API BackgroundService final : public QObject {
	Q_OBJECT

public:
	BackgroundService();

	~BackgroundService() override;

signals:
	void blurImage(const QImage& image);

	void readCdTrackInfo(const QString& disc_id, const std::forward_list<TrackInfo>& track_infos);

    void fetchDiscCoverCompleted(const QString& disc_id, const QString& cover_id);

	void fetchLyricsCompleted(const QList<SearchLyricsResult>& results);

	void fetchMbDiscInfoCompleted(const MbDiscIdInfo &info);

	void updateJobProgress(const QString& job_id, int new_progress);

	void jobError(const QString& job_id, const QString &message);

	void readAudioData(const std::vector<float>& interleaved);
public Q_SLOT:
	void cancelAllJob();

	void onAddJobs(const QString& dir_name, const QList<EncodeJob> &jobs);

	void cancelRequested();

	void onBlurImage(const QString& cover_id, const QPixmap& image, QSize size);

#if defined(Q_OS_WIN)
	void onFetchCdInfo(const DriveInfo& drive);
#endif

	void onSearchLyrics(const PlayListEntity& keyword);

	void parallelEncode(const QString& dir_name, QList<EncodeJob> jobs);

	void sequenceEncode(const QString& dir_name, QList<EncodeJob> jobs);

	void executeEncodeJob(const QString& dir_name, const EncodeJob& job);

	QCoro::Task<std::optional<QByteArray>> fetchCoverArtByUrl(const QString& tag, const QString& release_id, size_t prefer_size = 1200);
private:
	std::tuple<std::shared_ptr<FastIOStream>, Path> makeUniqueFile(const EncodeJob& job, const QString& dir_name);

	QCoro::Task<> searchLyrics(const PlayListEntity& keyword);

	QCoro::Task<std::optional<QByteArray>> tryFetch(const QString& tag, const QString& release_id, size_t size);

	bool is_stop_{false};
	LruCache<QString, QImage> blur_image_cache_;
	LoggerPtr logger_;
	std::stop_source stop_source_;
	QNetworkAccessManager nam_;
	http::HttpClient http_client_;
	std::shared_ptr<IThreadPool> thread_pool_;
	std::vector<std::unique_ptr<ILyricsSource>> lyrics_sources_;
};
