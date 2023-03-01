#pragma once

#include <QColor>
#include <QList>

class QImage;

using color_t = std::tuple<uint8_t, uint8_t, uint8_t>;

class ColorThief {
public:
	static QList<QColor> GetPalette(const QImage& image, int32_t color_count = 5, int32_t quality = 10, bool ignore_white = true);

private:
	static std::vector<color_t> GetPixels(const QImage& image, int32_t quality, bool ignore_white);
};
