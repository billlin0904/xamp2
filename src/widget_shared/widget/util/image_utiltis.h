//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <QPixmap>
#include <QImage>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

namespace image_utils {

inline constexpr int32_t kDarkAlpha = 80;
inline constexpr int32_t kImageRadius = 4;
inline constexpr int32_t kImageBlurRadius = 30;
inline constexpr int32_t kSmallImageRadius = 4;
inline constexpr int32_t kPlaylistImageRadius = 4;

XAMP_WIDGET_SHARED_EXPORT QPixmap roundImage(const QPixmap& src, int32_t radius = kImageRadius);

XAMP_WIDGET_SHARED_EXPORT QPixmap roundImage(const QPixmap& src, QSize size, int32_t radius = kImageRadius);

XAMP_WIDGET_SHARED_EXPORT QPixmap roundDarkImage(QSize size, int32_t alpha = kDarkAlpha, int32_t radius = kImageRadius);

XAMP_WIDGET_SHARED_EXPORT QPixmap resizeImage(const QPixmap& source, const QSize& size, bool is_aspect_ratio = false);

XAMP_WIDGET_SHARED_EXPORT Vector<uint8_t> image2Buffer(const QPixmap& source);

XAMP_WIDGET_SHARED_EXPORT QByteArray image2ByteArray(const QPixmap& source);

XAMP_WIDGET_SHARED_EXPORT QPixmap convertToImageFormat(const QPixmap& source, int32_t quality = 100);

XAMP_WIDGET_SHARED_EXPORT QImage blurImage(const QPixmap& source, QSize size);

XAMP_WIDGET_SHARED_EXPORT QPixmap gaussianBlur(const QPixmap& source, uint32_t radius = kImageBlurRadius);

XAMP_WIDGET_SHARED_EXPORT int sampleImageBlur(const QImage &image, int blur_alpha);

XAMP_WIDGET_SHARED_EXPORT QPixmap readFileImage(const QString& file_path, QSize size, QImage::Format format);

XAMP_WIDGET_SHARED_EXPORT bool moveFile(const QString& src_file_path, const QString& dest_file_path);

XAMP_WIDGET_SHARED_EXPORT bool optimizePng(const QByteArray& buffer, const QString& dest_file_path);

XAMP_WIDGET_SHARED_EXPORT QPixmap mergeImage(const QList<QPixmap>& images);

}
