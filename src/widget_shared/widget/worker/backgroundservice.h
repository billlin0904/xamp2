//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stop_token>

#include <QObject>
#include <base/object_pool.h>
#include <base/threadpoolexecutor.h>
#include <base/lrucache.h>

#include <base/charset_detector.h>

#include <widget/widget_shared.h>
#include <widget/playlistentity.h>
#include <widget/driveinfo.h>
#include <widget/widget_shared_global.h>
#include <widget/util/mbdiscid_util.h>
#include <widget/httpx.h>
#include <widget/encodejobwidget.h>
#include <widget/krcparser.h>

Q_DECLARE_METATYPE(ReplayGain);

struct LyricsParser {
	Candidate candidate;
	QSharedPointer<ILrcParser> parser;
	QByteArray content;
};

struct SearchLyricsResult {
	InfoItem info;	
	QList<LyricsParser> parsers;
};

Q_DECLARE_METATYPE(LyricsParser)
Q_DECLARE_METATYPE(SearchLyricsResult)

class XAMP_WIDGET_SHARED_EXPORT BackgroundService final : public QObject {
	Q_OBJECT

public:
	static constexpr size_t kBufferPoolSize = 256;

	BackgroundService();

	~BackgroundService() override;

signals:
	void blurImage(const QImage& image);

	void readCdTrackInfo(const QString& disc_id, const std::forward_list<TrackInfo>& track_infos);

    void fetchDiscCoverCompleted(const QString& disc_id, const QString& cover_id);

	void fetchLyricsCompleted(const QList<SearchLyricsResult>& results);

	void translationCompleted(const QString& keyword, const QString& result);

	void fetchMbDiscInfoCompleted(const MbDiscIdInfo &info);

	void updateJobProgress(const QString& job_id, int new_progress);

	void jobError(const QString& job_id, const QString &message);

	void readAudioSpectrogram(const QImage& spectrogram);

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

	void onSearchLyrics(const PlayListEntity& keyword);

	void onTranslation(const QString& keyword, const QString& from, const QString& to);

	void onReadWaveformAudioData(size_t frame_per_peek, const Path & file_path);

	void onReadSpectrogram(const QSize& widget_size, const Path& file_path);

	QCoro::Task<QList<SearchLyricsResult>> downloadKLrc(QList<InfoItem> infos);

	QCoro::Task<SearchLyricsResult> downloadSingleKlrc(InfoItem info);

private:
	bool is_stop_{false};
	LruCache<QString, QImage> blur_image_cache_;
	LoggerPtr logger_;
	std::stop_source stop_source_;
	QNetworkAccessManager nam_;
	http::HttpClient http_client_;
	LruCache<QString, SearchLyricsResult> lyrics_cache_;
	std::shared_ptr<IThreadPoolExecutor> thread_pool_;
};
