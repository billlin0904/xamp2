//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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

struct XAMP_WIDGET_SHARED_EXPORT ConstLatin1String final : public QLatin1String {
    constexpr ConstLatin1String(char const* const s) noexcept
        : QLatin1String(s, static_cast<int>(std::char_traits<char>::length(s))) {
    }

	constexpr ConstLatin1String(char const* const s, const int length) noexcept
		: QLatin1String(s, length) {
	}
};

class XAMP_WIDGET_SHARED_EXPORT StringView final : public std::string_view {
public:
	using std::string_view::string_view;

	QString utf16() const {
		return QString::fromUtf8(data(), size());
	}

	QByteArray utf8() const {
		return QByteArray::fromRawData(data(), size());
	}
};

namespace std {
	template <>
	struct hash<ConstLatin1String> {
		typedef size_t result_type;
		typedef ConstLatin1String argument_type;

		result_type operator()(const argument_type& s) const noexcept {
			return qHash(s);
		}
	};
}

inline constexpr ConstLatin1String kEmptyString{ "" };
inline constexpr ConstLatin1String kPlatformKey{ "windows" };

inline constexpr ConstLatin1String qTEXT(const char str[]) noexcept {
    return { str };
}

inline constexpr ConstLatin1String fromStdStringView(const std::string_view& s) noexcept {
	return { s.data(), static_cast<int>(s.length()) };
}

inline QString toQString(const std::optional<std::wstring>& s) {
	if (s) {
		return QString::fromStdWString(s.value());
	}
	return kEmptyString;
}

inline QString toQString(const std::optional<std::string>& s) {
	if (s) {
		return QString::fromStdString(s.value());
	}
	return kEmptyString;
}

XAMP_WIDGET_SHARED_EXPORT inline bool isNullOfEmpty(const QString& s) {
	return s.isNull() || s.isEmpty();
}

XAMP_WIDGET_SHARED_EXPORT inline QString qSTR(char const* const str) noexcept {
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