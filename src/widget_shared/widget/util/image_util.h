//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <QPixmap>
#include <QImage>
#include <QList>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

namespace image_util {

inline constexpr int32_t kDarkAlpha = 80;
inline constexpr int32_t kImageRadius = 4;
inline constexpr int32_t kImageBlurRadius = 30;
inline constexpr int32_t kSmallImageRadius = 4;
inline constexpr int32_t kPlaylistImageRadius = 4;
inline constexpr int32_t kCoverImageRadius = 10;

XAMP_WIDGET_SHARED_API QPixmap roundImage(const QPixmap& src, int32_t radius = kImageRadius);

XAMP_WIDGET_SHARED_API QPixmap roundImage(const QPixmap& src, QSize size, int32_t radius = kImageRadius);

XAMP_WIDGET_SHARED_API QPixmap roundCoverImage(const QPixmap& src, QSize size, int32_t radius = kCoverImageRadius);

XAMP_WIDGET_SHARED_API QPixmap roundDarkImage(QSize size, int32_t alpha = kDarkAlpha, int32_t radius = kImageRadius);

XAMP_WIDGET_SHARED_API QPixmap resizeImage(const QPixmap& source, const QSize& size, bool is_aspect_ratio = false);

XAMP_WIDGET_SHARED_API std::vector<uint8_t> image2JpegBuffer(const QPixmap& source, int32_t quality = 90);

XAMP_WIDGET_SHARED_API QImage blurImage(const std::shared_ptr<IThreadPool>& thread_pool, const QPixmap& source, QSize size);

XAMP_WIDGET_SHARED_API int sampleImageBlur(const QImage &image, int blur_alpha);

XAMP_WIDGET_SHARED_API QPixmap readFileImage(const QString& file_path, QSize size, QImage::Format format);

XAMP_WIDGET_SHARED_API QPixmap mergeImage(const QList<QPixmap>& images);

}
