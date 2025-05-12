//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QProcess>
#include <QCoroProcess>

#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT YtMusicServerProcessor {
public:
	YtMusicServerProcessor() = default;

	~YtMusicServerProcessor();

	QCoro::Task<int> start();
private:
	QProcess proc_;
};

