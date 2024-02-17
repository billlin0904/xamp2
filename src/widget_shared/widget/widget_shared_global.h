//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QtCore/qglobal.h>

#include <stdexcept>

#ifndef BUILD_STATIC
# if defined(WIDGET_SHARED_LIB)
#  define XAMP_WIDGET_SHARED_EXPORT Q_DECL_EXPORT
# else
#  define XAMP_WIDGET_SHARED_EXPORT Q_DECL_IMPORT
# endif
#else
# define XAMP_WIDGET_SHARED_EXPORT
#endif

#define tryLog(expr) \
    try {\
        [&, this]() mutable { expr; }();\
    }\
    catch (...) {\
        logAndShowMessage(std::current_exception());\
    }

XAMP_WIDGET_SHARED_EXPORT void logAndShowMessage(const std::exception_ptr& ptr);