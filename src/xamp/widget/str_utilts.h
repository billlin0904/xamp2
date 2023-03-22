//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QColor>
#include <QHashFunctions>

struct ConstLatin1String final : public QLatin1String {
    constexpr ConstLatin1String(char const* const s) noexcept
        : QLatin1String(s, static_cast<int>(std::char_traits<char>::length(s))) {
    }

	constexpr ConstLatin1String(char const* const s, int length) noexcept
		: QLatin1String(s, length) {
	}
};

class ConstString final : public std::string_view {
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

		result_type operator()(const argument_type& s) const {
			return qHash(s);
		}
	};
}

inline constexpr ConstLatin1String qEmptyString{ "" };

constexpr ConstLatin1String qTEXT(const char str[]) noexcept {
    return { str };
}

constexpr ConstLatin1String FromStdStringView(std::string_view const& s) noexcept {
	return { s.data(), static_cast<int>(s.length()) };
}

inline QString qSTR(char const* const str) noexcept {
    return {QLatin1String{ str }};
}

QString FormatSampleRate(uint32_t sample_rate);

QString FormatBitRate(uint32_t bitRate);

QString FormatDsdSampleRate(uint32_t dsd_speed);

QString ColorToString(QColor color);

QString BackgroundColorToString(QColor color);

QString FormatDuration(const double stream_time, bool full_text = false);

bool IsMoreThan1Hours(const double stream_time);

QString ToNativeSeparators(const QString &path);

QByteArray GenerateUuid();

QString FormatBytes(quint64 bytes);

QString FormatTime(quint64 time);

QString FormatDb(double value, int prec = 1);

QString FormatDouble(double value, int prec = 1);