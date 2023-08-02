//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/str_utilts.h>
#include <QNetworkRequest>
#include <QObject>

class HttpRequest : public QNetworkRequest {
public:
	QString GetHost() const {
		return url_.host();
	}

	int GetPort() const {
		if (url_.scheme() == qTEXT("https")) {
			return url_.port(443);
		}
		return url_.port(80);
	}

	QPair<QString, int> GetHostKey() const {
		return qMakePair(GetHost(), GetPort());
	}

private:
	QUrl url_;
};

class RequestPriorityQueue {
public:

};

class HttpService : public QObject {
public:
	explicit HttpService(QObject* parent);
};
