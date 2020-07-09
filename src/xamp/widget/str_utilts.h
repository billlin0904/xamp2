//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string_view>

#include <QColor>
#include <QString>

struct ConstLatin1String : public QLatin1String {
    constexpr ConstLatin1String(char const* const s)
        : QLatin1String(s, static_cast<int>(std::char_traits<char>::length(s))) {
    }
};

namespace Qt {
    static constexpr ConstLatin1String EmptyStr{ "" };
}

#define Q_UTF8(str) QLatin1String{str}
#define Q_STR(str) QStringLiteral(str)

inline QLatin1String fromStdStringView(std::string_view const &s) {
    return QLatin1String{ s.data(), static_cast<int>(s.length()) };
}

inline QString colorToString(QColor color) noexcept {
    return QString().sprintf("rgba(%d,%d,%d,%d)", 
        color.red(), color.green(), color.blue(), color.alpha());
}


inline QString backgroundColorToString(QColor color) {
    return Q_UTF8("background-color: ") + colorToString(color) + Q_UTF8(";");
}
