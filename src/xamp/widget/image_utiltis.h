//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QPainter>
#include <QPixmap>

namespace Pixmap {

inline constexpr int32_t kImageRadius = 5;
inline constexpr int32_t kSmallImageRadius = 10;
inline constexpr int32_t kPlaylistImageRadius = 2;

QPixmap roundImage(const QPixmap& src, int32_t radius = kImageRadius);

QPixmap roundImage(const QPixmap& src, QSize size, int32_t radius = kImageRadius);

QPixmap resizeImage(const QPixmap& source, const QSize& size, bool is_aspect_ratio = false);

std::vector<uint8_t> getImageDate(const QPixmap& source);

}
