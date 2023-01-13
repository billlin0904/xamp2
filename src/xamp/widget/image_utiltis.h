//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <QPixmap>
#include <QImage>

namespace ImageUtils {

inline constexpr int32_t kImageRadius = 4;
inline constexpr int32_t kSmallImageRadius = 8;
inline constexpr int32_t kPlaylistImageRadius = 2;

QPixmap roundImage(const QPixmap& src, int32_t radius = kImageRadius);

QPixmap roundImage(const QPixmap& src, QSize size, int32_t radius = kImageRadius);

QPixmap roundDarkImage(QSize size, int32_t alpha = 80, int32_t radius = kImageRadius);

QPixmap resizeImage(const QPixmap& source, const QSize& size, bool is_aspect_ratio = false);

std::vector<uint8_t> convert2Vector(const QPixmap& source);

QByteArray convert2ByteArray(const QPixmap& source);

QImage blurImage(const QPixmap& source, QSize size);

int sampleImageBlur(const QImage &image, int blur_alpha);

bool optimizePNG(const QString& src_file_path, const QString& dest_file_path);

bool optimizePNG(const QByteArray& buffer, const QString& dest_file_path);

size_t getPixmapSize(const QPixmap &pixmap);

}
