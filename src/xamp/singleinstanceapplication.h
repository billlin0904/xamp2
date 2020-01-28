//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QDebug>
#include <QLocalSocket>
#include <QLocalServer>
#include <QtWidgets/QApplication>

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
};
