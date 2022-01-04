//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

struct YtMusicMediaEntity {
    bool isPlaying{false};
    int32_t length{0};
    int32_t progress{0};
    QString title;
    QString by;
    QString thumbnail;
};

Q_DECLARE_METATYPE(YtMusicMediaEntity)

class YtMusicObserver : public QObject {
	Q_OBJECT
public:
	explicit YtMusicObserver(QObject* parent);

	Q_INVOKABLE void postMessage(const QString &json);

signals:
    void updateMediaEntity(const YtMusicMediaEntity &entity);
};

