//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QTime>

#include "str_utilts.h"

namespace Time {

inline QString msToString(const double stream_time) {
	const auto ms = int32_t(stream_time * 1000.0) % 1000;
	const auto secs = static_cast<int32_t>(stream_time);
	const auto h = secs / 3600;
	const auto m = (secs % 3600) / 60;
	const auto s = (secs % 3600) % 60;
	QTime t(h, m, s, ms);
	if (h > 0) {
		return t.toString(Q_UTF8("hh:mm:ss"));
	}
	return t.toString(Q_UTF8("mm:ss"));
}

}
