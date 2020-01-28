//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/http.h>

class DiscogsClient : public QObject {
    Q_OBJECT
public:
    DiscogsClient(QObject *parent = nullptr);

    void searchArtist(int32_t artist_id, const QString &artist);
};
