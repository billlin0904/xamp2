//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QPainter>
#include <QPixmap>

namespace Pixmap {

static QPixmap resizeImage(const QPixmap &source, const QSize &size, bool is_aspect_ratio = false) {
    return source.scaled(size, 
		is_aspect_ratio ? Qt::KeepAspectRatioByExpanding 
		: Qt::IgnoreAspectRatio, 
		Qt::SmoothTransformation);
}

}