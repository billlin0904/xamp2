//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/http.h>
#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <widget/util/str_util.h>

#include <base/expected.h>

struct XAMP_WIDGET_SHARED_EXPORT OAuthToken {
	int64_t expires_in{ 0 };
	int64_t expires_at{ 0 };
	QString filepath;
	QString access_token;
	QString refresh_token;
	QString scope;
	QString token_type;
};

class XAMP_WIDGET_SHARED_EXPORT YtMusicOAuth : public QObject {	
	Q_OBJECT
public:
	YtMusicOAuth();

	void setup();

	void requestGrant();

	static Expected<OAuthToken, std::string> parseOAuthJson();
signals:
	void requestGrantCompleted();

	void acceptAuthorization(const QString &url);

	void setupError();

	void requestGrantError();

private:
	QString user_code_;
	QString verification_url_;
	QString device_code_;
	LoggerPtr logger_;
};

