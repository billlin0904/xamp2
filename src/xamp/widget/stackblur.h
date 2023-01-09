//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <QImage>

#include <widget/widget_shared.h>

class Stackblur final {
public:
	Stackblur(IThreadPoolExecutor& tp, QImage& image, uint32_t radius);

private:
	void blur(IThreadPoolExecutor& tp, uint8_t* src, uint32_t width, uint32_t height, uint32_t radius, uint32_t thread_branch);
};