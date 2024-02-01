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
	Q_OBJECT
public:
	YtMusicOAuth();

	void setup();

	void requestGrant();

signals:
	void requestGrantCompleted();

	void acceptAuthorization();

private:
	QString user_code_;
	QString verification_url_;
	QString device_code_;
	LoggerPtr logger_;
};

