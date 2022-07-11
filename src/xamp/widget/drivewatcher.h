//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QTimer>
#include <QFileInfoList>

#include <widget/driveinfo.h>

class DriveWatcher : public QObject {
	Q_OBJECT
public:
	explicit DriveWatcher(QObject* parent = nullptr);

signals:
	void drivesChanges(const QList<DriveInfo> &drive_infos);

private:
	void checkForDriveChanges();

	void readDriveInfo();

	QTimer timer_;
	QFileInfoList file_info_list_;
};
