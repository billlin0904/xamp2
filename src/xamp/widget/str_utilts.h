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

#define Q_UTF8(str) ConstLatin1String{str}
#define Q_STR(str) QString(QLatin1String(str))

inline QLatin1String fromStdStringView(std::string_view const &s) {
    return QLatin1String{ s.data(), static_cast<int>(s.length()) };
}

