//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QPainter>
#include <QPixmap>

namespace Pixmap {

class Stackblur {
public:
	Stackblur(QImage& image, uint32_t radius);

private:
	void blur(uint8_t *src, uint32_t width, uint32_t height, uint32_t radius, int32_t cores = 1);
};

inline QPixmap resizeImage(const QPixmap &source, const QSize &size, bool is_aspect_ratio = false) {
    return source.scaled(size, 
		is_aspect_ratio ? Qt::KeepAspectRatioByExpanding 
		: Qt::IgnoreAspectRatio, 
		Qt::SmoothTransformation);
}

inline QPixmap blurImage(const QPixmap& source, uint32_t radius = 30) {
	auto img = source.toImage();
	Stackblur s(img, radius);
	return QPixmap::fromImage(img);
}

}
