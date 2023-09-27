//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <QPixmap>
#include <QImage>
#include <widget/widget_shared_global.h>

namespace image_utils {

inline constexpr int32_t kImageRadius = 4;
inline constexpr int32_t kSmallImageRadius = 4;
inline constexpr int32_t kPlaylistImageRadius = 4;

XAMP_WIDGET_SHARED_EXPORT QPixmap RoundImage(const QPixmap& src, int32_t radius = kImageRadius);

XAMP_WIDGET_SHARED_EXPORT QPixmap RoundImage(const QPixmap& src, QSize size, int32_t radius = kImageRadius);

XAMP_WIDGET_SHARED_EXPORT QPixmap RoundDarkImage(QSize size, int32_t alpha = 80, int32_t radius = kImageRadius);

XAMP_WIDGET_SHARED_EXPORT QPixmap ResizeImage(const QPixmap& source, const QSize& size, bool is_aspect_ratio = false);

XAMP_WIDGET_SHARED_EXPORT std::vector<uint8_t> Image2ByteVector(const QPixmap& source);

XAMP_WIDGET_SHARED_EXPORT QByteArray Image2ByteArray(const QPixmap& source);

XAMP_WIDGET_SHARED_EXPORT QImage BlurImage(const QPixmap& source, QSize size);

XAMP_WIDGET_SHARED_EXPORT int SampleImageBlur(const QImage &image, int blur_alpha);

XAMP_WIDGET_SHARED_EXPORT bool OptimizePng(const QString& src_file_path, const QString& dest_file_path);

XAMP_WIDGET_SHARED_EXPORT bool OptimizePng(const QByteArray& buffer, const QString& dest_file_path);

}
