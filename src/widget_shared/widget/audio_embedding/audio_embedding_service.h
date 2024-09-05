//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QtConcurrent/QtConcurrent>
#include <QObject>
#include <QFuture>
#include <QScopedPointer>
#include <widget/util/str_util.h>
#include <widget/http.h>
#if 0

#include <base/lazy_storage.h>
#include <widget/widget_shared.h>
#include <widget/util/str_util.h>
#include <base/enum.h>
#include <base/stl.h>
#include <widget/widget_shared_global.h>
#include <widget/baseservice.h>

struct XAMP_WIDGET_SHARED_EXPORT AudioEmbeddingQueryResult {
	std::string file_path;
	float distances{0};
};

class XAMP_WIDGET_SHARED_EXPORT AudioEmbeddingDataBaseInterop {
public:
	AudioEmbeddingDataBaseInterop();

	XAMP_PIMPL(AudioEmbeddingDataBaseInterop)

	bool initial();

	bool embedAndSave(const std::string& path);

	std::vector<AudioEmbeddingQueryResult> queryEmbeddings(const std::vector<std::string>& paths, int32_t n_results = 5);
private:
	class AudioEmbeddingDataBaseInteropImpl;
	AlignPtr<AudioEmbeddingDataBaseInteropImpl> impl_;
};

class XAMP_WIDGET_SHARED_EXPORT AudioEmbeddingService : public BaseService {
	Q_OBJECT
public:
	explicit AudioEmbeddingService(QObject* parent = nullptr);

	QFuture<bool> initialAsync();

	QFuture<bool> cleanupAsync();

	QFuture<bool> embedAndSave(const std::string& path);

	void cancelRequested();

	QFuture<std::vector<AudioEmbeddingQueryResult>> queryEmbeddings(const std::vector<std::string>& paths, int32_t n_results);
private:
	AudioEmbeddingDataBaseInterop* interop();

	LocalStorage<AudioEmbeddingDataBaseInterop> interop_;
};

#endif

namespace http {
class HttpClient;
}

struct XAMP_WIDGET_SHARED_EXPORT EmbeddingQueryResult {
    QString file_path;
    QList<float> distances;
    QString audio_id;
};

class XAMP_WIDGET_SHARED_EXPORT AudioEmbeddingService : public QObject {
    Q_OBJECT
public:
    explicit AudioEmbeddingService(QObject* parent = nullptr);

    ~AudioEmbeddingService() override;

    void embedAndSave(const QString& path, int32_t audio_id);

    void queryEmbeddings(const QList<QString> &paths);

signals:
    void queryEmbeddingsReady(const QList<EmbeddingQueryResult> &results);

private:
};
