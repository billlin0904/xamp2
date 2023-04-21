//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMetaType>
#include <widget/widget_shared_global.h>

struct XAMP_WIDGET_SHARED_EXPORT DriveInfo {
	char driver_letter;
	QString display_name;
	QString drive_path;
};

Q_DECLARE_METATYPE(DriveInfo)

