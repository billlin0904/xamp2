//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/http.h>

class MusicBrainzClient : public QObject {
    Q_OBJECT
public:
    explicit MusicBrainzClient(QObject *parnet = nullptr);

    void searchArtist(int32_t artist_id, const QString &artist);

    void getArtistImageUrl(const QString &mbid);

signals:
    void onGetArtistMBID(int32_t artist_id, const QString &mbid);

    void onGetArtistImageUrl(int32_t artist_id, const QString &image_url);
};
