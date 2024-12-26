//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QColor>
#include <QHashFunctions>
#include <QCoreApplication>
#include <QVersionNumber>

#include <base/str_utilts.h>
#include <widget/widget_shared_global.h>

struct XAMP_WIDGET_SHARED_EXPORT ConstexprQString final : public QLatin1String {
    constexpr ConstexprQString(char const* const s) noexcept
        : QLatin1String(s, static_cast<int>(std::char_traits<char>::length(s))) {
    }

	constexpr ConstexprQString(char const* const s, const int length) noexcept
		: QLatin1String(s, length) {
	}
};

inline constexpr ConstexprQString operator"" _str(const char* s, std::size_t length) noexcept {
	return ConstexprQString(s, static_cast<int>(length));
}

namespace std {
	template <>
	struct hash<ConstexprQString> {
		typedef size_t result_type;
		typedef ConstexprQString argument_type;

		result_type operator()(const argument_type& s) const noexcept {
			return qHash(s);
		}
	};
}

inline constexpr ConstexprQString kEmptyString{ "" };
inline constexpr ConstexprQString kPlatformKey{ "windows" };

inline constexpr ConstexprQString fromStdStringView(const std::string_view& s) noexcept {
	return { s.data(), static_cast<int>(s.length()) };
}

inline QString optStrToQString(const std::optional<std::wstring>& s) {
	if (s) {
		return QString::fromStdWString(s.value());
	}
	return kEmptyString;
}

inline QString optStrToQString(const std::optional<std::string>& s) {
	if (s) {
		return QString::fromStdString(s.value());
	}
	return kEmptyString;
}

inline QString toQString(const std::wstring& s) {
	return QString::fromStdWString(s);
}

inline QString toQString(const std::string& s) {
	return QString::fromStdString(s);
}

XAMP_WIDGET_SHARED_EXPORT inline bool isNullOfEmpty(const QString& s) {
	return s.isNull() || s.isEmpty();
}

XAMP_WIDGET_SHARED_EXPORT inline QString qFormat(const QString &str) {
	return str;
}

XAMP_WIDGET_SHARED_EXPORT inline QString qFormat(char const* const str) {
    return {QLatin1String{ str }};
}

XAMP_WIDGET_SHARED_EXPORT QString formatSampleRate(uint32_t sample_rate);

XAMP_WIDGET_SHARED_EXPORT QString formatBitRate(uint32_t bit_rate);

XAMP_WIDGET_SHARED_EXPORT QString colorToString(QColor color);

XAMP_WIDGET_SHARED_EXPORT QString formatDuration(const double stream_time, bool full_text = false);

XAMP_WIDGET_SHARED_EXPORT bool isMoreThan1Hours(const double stream_time);

XAMP_WIDGET_SHARED_EXPORT QString toNativeSeparators(const QString& path);

XAMP_WIDGET_SHARED_EXPORT bool parseVersion(const QString& s, QVersionNumber& version);

XAMP_WIDGET_SHARED_EXPORT QString formatVersion(const QVersionNumber& version);

QString formatDsdSampleRate(uint32_t dsd_speed);

QString backgroundColorToString(QColor color);

QByteArray generateUuid();

QString formatBytes(quint64 bytes);

QString formatTime(quint64 time);

QString formatDb(double value, int prec = 1);

QString formatDouble(double value, int prec = 1);

XAMP_WIDGET_SHARED_EXPORT double parseDuration(const std::string& str);

template <typename... Args>
QString stringFormat(std::string_view s, Args &&...args) {
	using namespace xamp::base::String;
	return QString::fromStdString(Format(s, args...));
}
