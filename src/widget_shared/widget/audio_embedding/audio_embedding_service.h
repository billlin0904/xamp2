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

    void embedAndSave(const QString& path, int32_t audio_id);

    void queryEmbeddings(const QList<QString> &paths);

    void flush();
signals:
    void queryEmbeddingsReady(const QList<EmbeddingQueryResult> &results);
};
