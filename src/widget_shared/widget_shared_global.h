#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(WIDGET_SHARED_LIB)
#  define WIDGET_SHARED_EXPORT Q_DECL_EXPORT
# else
#  define WIDGET_SHARED_EXPORT Q_DECL_IMPORT
# endif
#else
# define WIDGET_SHARED_EXPORT
#endif
