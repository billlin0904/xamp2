//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <base/logger_impl.h>
#include <widget/widget_shared_global.h>

namespace log_util {

using xamp::base::LogLevel;

XAMP_WIDGET_SHARED_EXPORT QString getLogLevelString(LogLevel level);
XAMP_WIDGET_SHARED_EXPORT LogLevel parseLogLevel(const QString& str);

}
