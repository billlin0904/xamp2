//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared_global.h>
#include <widget/util/str_util.h>

class XAMP_WIDGET_SHARED_EXPORT M3uParser {
public:
    static bool isPlaylistFilenameSupported(const QString& fileName);

    static QList<QString> parseM3UFile(const QString& file_name);

    static bool writeM3UFile(const QString& file_name,
        const QList<QString>& items, 
        const QString& playlist = QString(),
        bool useRelativePath = false,
        bool utf8_encoding = true);
};

