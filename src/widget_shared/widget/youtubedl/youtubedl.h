//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QJsonObject>
#include <QProcess>
#include <QVector>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

class QProcess;
class QString;

struct XAMP_WIDGET_SHARED_EXPORT MediaFormat {
    QString format_id;
    QString format;
    QString extension;
    QString resolution;
    QString quality;
    QString note;
    QString acodec;
    QString vcodec;
};

class XAMP_WIDGET_SHARED_EXPORT YoutubeDL : public QObject {
    Q_OBJECT
public:
    YoutubeDL();

    QJsonObject createJsonObject(const QString &url);

    void availableFormats(const QString& url);

    QString getUrl(const QString& url);

    const QProcess* process() const;

    void resetArguments();

    void setFormat(const QString& format);

    void startDownload(const QString& url, const QString& working_directory);

    QVector<MediaFormat> formats() const;

    void setFormats(const QVector<MediaFormat>& value);

    void addArguments(const QString& arg);

public slots:
    void onReadyRead();

    void onDownloadFinished(int exit_code, QProcess::ExitStatus exit_status);

private:
    QStringList arguments_;
    QVector<MediaFormat> formats_;
    QString program_;
    QProcess* process_;
    LoggerPtr logger_;
};