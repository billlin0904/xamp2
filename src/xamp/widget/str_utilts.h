//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>

struct ConstLatin1String : public QLatin1String {
    constexpr ConstLatin1String(char const* const s)
        : QLatin1String(s, static_cast<int>(std::char_traits<char>::length(s))) {
    }
};

namespace Qt {
    inline constexpr ConstLatin1String EmptyString{ "" };
}

#define Q_UTF8(str) ConstLatin1String{str}
#define Q_STR(str) QString(QLatin1String(str))

inline QLatin1String fromStdStringView(std::string_view const &s) {
    return QLatin1String{ s.data(), static_cast<int>(s.length()) };
}

inline QString samplerate2String(uint32_t samplerate) {
    auto precision = 1;
    auto is_mhz_samplerate = false;
    if (samplerate / 1000 > 1000) {
        is_mhz_samplerate = true;
    }
    else {
        precision = samplerate % 1000 == 0 ? 0 : 1;
    }

    return (is_mhz_samplerate ? QString::number(samplerate / 1000000.0, 'f', 2) + Q_UTF8("MHz")
        : QString::number(samplerate / 1000.0, 'f', precision) + Q_UTF8("kHz"));
}
