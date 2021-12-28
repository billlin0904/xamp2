//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QDebug>
#include <QLocalSocket>
#include <QLocalServer>
#include <QApplication>

#ifdef Q_OS_WIN
#include <base/windows_handle.h>
#endif

class SingleInstanceApplication : public QObject {
	Q_OBJECT
public:
    SingleInstanceApplication();

	bool attach(const QStringList& args);
	
signals:
	void newInstanceDetected();

public slots:
	void onNewConnection();

	void readyRead();

private:
	bool listen(const QString &serverName);

	QLocalSocket* socket_;
	QLocalServer server_;
#ifdef Q_OS_WIN
	xamp::base::WinHandle mutex_;
#endif
};
