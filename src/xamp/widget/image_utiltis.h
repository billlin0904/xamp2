//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QPainter>
#include <QPixmap>

namespace Pixmap {

QPixmap resizeImage(const QPixmap& source, const QSize& size, bool is_aspect_ratio = false);

QPixmap blurImage(const QPixmap& source, uint32_t radius);

std::vector<uint8_t> getImageDate(const QPixmap& source);

}
