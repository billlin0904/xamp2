//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>

namespace win32 {
	struct GdiPlusInit {
		GdiPlusInit();
		~GdiPlusInit();
		unsigned long long token;
	};

	void setBlurMaterial(const QWidget* widget, bool enable);
	void setFramelessWindowStyle(const QWidget* widget);
	void drawDwmShadow(const QWidget* widget);
	void drawGdiShadow(void* message);
	void removeStandardFrame(void* message);
	void setResizeable(void* message);
}

