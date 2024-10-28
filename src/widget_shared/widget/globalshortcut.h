//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

#include <qwindowdefs.h>

#include <base/memory.h>
#include <base/base.h>

class XAMP_WIDGET_SHARED_EXPORT GlobalShortcut final {
public:
    GlobalShortcut();

    XAMP_PIMPL(GlobalShortcut)

	bool registerShortcut(const WId wid, quint32 native_key, quint32 native_mods);

    bool unregisterShortcut(const WId wid, quint32 native_key, quint32 native_mods);

    quint32 nativeModifiers(Qt::KeyboardModifiers modifiers);

    quint32 nativeKeycode(Qt::Key key);
private:
    class GlobalShortcutImpl;
    ScopedPtr<GlobalShortcutImpl> impl_;
};

#define qGlobalShortcut SharedSingleton<GlobalShortcut>::GetInstance()
