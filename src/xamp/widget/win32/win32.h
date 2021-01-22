//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>

namespace win32 {
	void setBlurMaterial(const QWidget* widget, bool enable, bool use_native_window);
	void setWinStyle(QWidget* widget);
}

