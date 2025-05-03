//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QtCore/qglobal.h>
#include <base/port.h>
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


XAMP_WIDGET_SHARED_EXPORT void logAndShowMessage(const std::exception_ptr& ptr);

template <typename Func>
void tryLog(Func&& func) {
    try {
        func();
    }
    catch (...) {
        logAndShowMessage(std::current_exception());
    }
}

#define XAMP_TRY_LOG(expr) tryLog([&]() mutable { expr; })