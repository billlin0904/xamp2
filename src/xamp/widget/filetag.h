//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QByteArray>

namespace FileTag {
	QString getTagId(const QByteArray &buffer);
	QString getTagId(const QString &file_name);
};

