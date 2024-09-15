//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QtConcurrent/QtConcurrent>
#include <QCoroTask>
#include <QObject>
#include <QFuture>
#include <QScopedPointer>
#include <widget/util/str_util.h>
#include <widget/httpx.h>

struct XAMP_WIDGET_SHARED_EXPORT EmbeddingQueryResult {
    QString file_path;
    double distances;
    QString audio_id;
};

class XAMP_WIDGET_SHARED_EXPORT AudioEmbeddingService : public QObject {
    Q_OBJECT
public:
    explicit AudioEmbeddingService(QObject* parent = nullptr);

    ~AudioEmbeddingService() override;

    QCoro::Task<QString> embedAndSave(const QString& path, int32_t audio_id);

    QCoro::Task<QList<EmbeddingQueryResult>> queryEmbeddings(const QList<QString> &paths);

    QCoro::Task<QString> flush();
private:
	http::HttpClient http_client_;
};
