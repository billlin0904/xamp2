//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QColor>

struct ConstLatin1String final : public QLatin1String {
    constexpr ConstLatin1String(char const* const s) noexcept
        : QLatin1String(s, static_cast<int>(std::char_traits<char>::length(s))) {
    }
};

namespace Qt {
    inline constexpr ConstLatin1String EmptyString{ "" };
}

constexpr ConstLatin1String Q_TEXT(const char str[]) noexcept {
    return ConstLatin1String{ str };
}

constexpr ConstLatin1String fromStdStringView(std::string_view const& s) noexcept {
    return ConstLatin1String{ s.data() };
}

inline QString Q_STR(char const* const str) noexcept {
    return {QLatin1String{ str }};
}

QString samplerate2String(uint32_t samplerate);

QString bitRate2String(uint32_t bitRate);

QString dsdSampleRate2String(uint32_t dsd_speed);

QString colorToString(QColor color);

QString backgroundColorToString(QColor color);

QString msToString(const double stream_time, bool full_text = false);

bool isMoreThan1Hours(const double stream_time);