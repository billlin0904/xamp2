//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT TabPage {
public:
	virtual ~TabPage() = default;
	virtual void reload() = 0;
protected:
	TabPage() = default;
};

