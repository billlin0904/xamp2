//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/http.h>
#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <widget/util/str_utilts.h>

class XAMP_WIDGET_SHARED_EXPORT YtMusicOAuth : public QObject {
	static constexpr auto kOAuthCodeUrl = qTEXT("https://www.youtube.com/o/oauth2/device/code");
	static constexpr auto kOAuthScope = qTEXT("https://www.googleapis.com/auth/youtube");

	static constexpr auto kOAuthTokenUrl = qTEXT("https://oauth2.googleapis.com/token");
	static constexpr auto kOAuthClientSecret = qTEXT("SboVhoG9s0rNafixCSGGKXAT");

	static constexpr auto kOAuthClientId = 
		qTEXT("861556708454-d6dlm3lh05idd8npek18k6be8ba3oc68.apps.googleusercontent.com");

	static constexpr auto kOAuthUserAgent = 
		qTEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:88.0) Gecko/20100101 Firefox/88.0 Cobalt/Version");
public:
	YtMusicOAuth();

	void setup(const QString& file_name);
private:
	LoggerPtr logger_;
};

