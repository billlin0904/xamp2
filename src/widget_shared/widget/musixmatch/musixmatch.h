//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/util/str_util.h>
#include <widget/httpx.h>
#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT MusixmatchHttpService {
public:
	MusixmatchHttpService();

	QCoro::Task<QString> ensureUserToken();

	QCoro::Task<QString> search(const QString& q_album, const QString& q_artist, const QString& q_title);

private:
	QString covertToLrcString(const QString& content);

	QCoro::Task<QString> getUserToken();

	QString user_token_;
	http::HttpClient http_client_;
};

