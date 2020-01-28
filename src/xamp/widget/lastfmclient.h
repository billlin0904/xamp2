//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/http.h>

#include <QObject>
#include <QPixmap>

class LastfmClient : public QObject {
    Q_OBJECT
public:
    LastfmClient(QObject *parent = nullptr);

    void searchArtist(int32_t artist_id, const QString &artist);

    void downloadImage(int32_t artist_id, const QString &image_url);

signals:
    void onGetArtistImageUrl(int32_t artist_id, const QString &image_url);

    void onDownloadImage(int32_t artist_id, const QPixmap &image);
};
