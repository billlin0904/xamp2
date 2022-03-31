//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QMenu>

namespace win32 {
	void setBlurMaterial(const QWidget* widget, bool enable, int animation_id = 0);
	void setBlurMaterial(const QMenu* menu, bool enable, int animation_id = 0);
	void setFramelessWindowStyle(const QWidget* widget);
	void drawDwmShadow(const QWidget* widget);
	void drawDwmShadow(const QMenu* menu);
	void removeStandardFrame(void* message);
	bool compositionEnabled();
	void setResizeable(void* message);
	bool isWindowMaximized(const QWidget* widget);
}

