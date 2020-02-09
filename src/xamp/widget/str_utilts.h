//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>

#define Q_UTF8(str) QLatin1String{str}
#define Q_EMPTY_STR QLatin1String{""}

inline QString formatBytes(size_t bytes) noexcept {
    constexpr float tb = 1099511627776;
    constexpr float gb = 1073741824;
    constexpr float mb = 1048576;
    constexpr float kb = 1024;

    QString result;

    if (bytes >= tb)
        result.sprintf("%.2f TB", (float)bytes / tb);
    else if (bytes >= gb && bytes < tb)
        result.sprintf("%.2f GB", (float)bytes / gb);
    else if (bytes >= mb && bytes < gb)
        result.sprintf("%.2f MB", (float)bytes / mb);
    else if (bytes >= kb && bytes < mb)
        result.sprintf("%.2f KB", (float)bytes / kb);
    else if (bytes < kb)
        result.sprintf("%.2f Bytes", (float)bytes);
    else
        result.sprintf("%.2f Bytes", (float)bytes);

    return result;
}

