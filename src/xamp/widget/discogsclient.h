//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QNetworkAccessManager>

#include <widget/http.h>

class DiscogsClient : public QObject {
    Q_OBJECT
public:
    DiscogsClient(QNetworkAccessManager* manager = nullptr, QObject *parent = nullptr);

    void searchArtist(int32_t artist_id, const QString &artist);

    void searchArtistId(int32_t artist_id, const QString& id);

    void downloadArtistImage(int32_t artist_id, const QString& url);

signals:
    void getArtistId(int32_t artist_id, const QString &id);

    void getArtistImageUrl(int32_t artist_id, const QString& url);

    void downloadImageFinished(int32_t artist_id, const QPixmap &image);

public slots:

private:
    QNetworkAccessManager* manager_;
};
