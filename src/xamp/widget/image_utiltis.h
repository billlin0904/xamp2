//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <QPixmap>
#include <QImage>

namespace image_utils {

inline constexpr int32_t kImageRadius = 4;
inline constexpr int32_t kSmallImageRadius = 10;
inline constexpr int32_t kPlaylistImageRadius = 2;

QPixmap RoundImage(const QPixmap& src, int32_t radius = kImageRadius);

QPixmap RoundImage(const QPixmap& src, QSize size, int32_t radius = kImageRadius);

QPixmap RoundDarkImage(QSize size, int32_t alpha = 80, int32_t radius = kImageRadius);

QPixmap ResizeImage(const QPixmap& source, const QSize& size, bool is_aspect_ratio = false);

std::vector<uint8_t> Pixmap2ByteVector(const QPixmap& source);

QByteArray Pixmap2ByteArray(const QPixmap& source);

QImage BlurImage(const QPixmap& source, QSize size);

QImage AcrylicImage(const QImage& source,
	const QColor& tint_color,
	const QColor& luminosity_color = QColor(255, 255, 255, 0),
	qreal noise_opacity = 0.03);

int SampleImageBlur(const QImage &image, int blur_alpha);

bool OptimizePng(const QString& src_file_path, const QString& dest_file_path);

bool OptimizePng(const QByteArray& buffer, const QString& dest_file_path);

void OptimizePng(const QByteArray& buffer, std::vector<uint8_t>& result_png);

}
