//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>

#include <QString>
#include <QTime>

#include <widget/str_utilts.h>

namespace Time {

static QString msToString(const double stream_time) {
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

static double toDoubleTime(const QTime &time) {
	std::chrono::minutes m(time.minute());
	std::chrono::seconds s(time.second());
	std::chrono::milliseconds ms(time.msec());
	std::chrono::milliseconds result(m + s + ms);
	return result.count() / 1000.0;
}

static QTime toQTime(const double stream_time) {
	return QTime::fromString(Q_UTF8("00:") + msToString(stream_time));
}

}
