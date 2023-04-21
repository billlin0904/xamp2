//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(WIDGET_SHARED_LIB)
#  define XAMP_WIDGET_SHARED_EXPORT Q_DECL_EXPORT
# else
#  define XAMP_WIDGET_SHARED_EXPORT Q_DECL_IMPORT
# endif
#else
# define XAMP_WIDGET_SHARED_EXPORT
#endif
