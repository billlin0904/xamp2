//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

class YtMusicObserver : public QObject {
	Q_OBJECT
public:
	explicit YtMusicObserver(QObject* parent);

	Q_INVOKABLE void postMessage(const QString &json);

signals:
	void receiveMessage();
};

