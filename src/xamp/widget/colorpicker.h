//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QImage>
#include <QPixmap>

class ColorPicker {
public:
	ColorPicker();

	void loadImage(const QPixmap& image);
	void loadImage(const QImage& image);

	QImage getTestImage(const QImage& image) const;
private:

};
