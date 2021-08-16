//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QNetworkAccessManager>

#include <widget/http.h>
#include <widget/str_utilts.h>

class MusixmatchClient final : public QObject {
	Q_OBJECT
public:
	explicit MusixmatchClient(QNetworkAccessManager* manager = nullptr, QObject* parent = nullptr);

	QString getUrl(QString const &url) const;

	void matcherLyrics(QString const & title,
		QString const& artist,
		QString format = Q_UTF8("json"));
	
private:
	QString api_key_;
	QNetworkAccessManager* manager_;
};
