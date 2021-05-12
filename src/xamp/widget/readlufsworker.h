//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QThread>
#include <QObject>
#include <QQueue>

struct PlayListEntity;

class ReadLufsWorker : public QObject {
	Q_OBJECT
public:	
	ReadLufsWorker();

signals:
	void readCompleted(int32_t music_id, double lrus, double trure_peak);
	
public slots:
	void addEntity(PlayListEntity const& entity);
};

