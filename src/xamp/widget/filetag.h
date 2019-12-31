//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QByteArray>

class FileTag {
public:
	static QString getTagId(const QByteArray &buffer);
	static QString getTagId(const QString &file_name);
};

