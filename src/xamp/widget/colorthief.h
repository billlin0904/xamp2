#pragma once

#include <QColor>
#include <QList>

class QImage;

using QuantizedColor = std::tuple<uint8_t, uint8_t, uint8_t>;

class ColorThief {
public:
	ColorThief() = default;

	void LoadImage(const QImage& image, int32_t color_count = 5, int32_t quality = 10, bool ignore_white = true);

	const QList<QColor>& GetPalette() const;

	QColor GetDominantColor() const;

private:
	static std::vector<QuantizedColor> GetPixels(const QImage& image, int32_t quality, bool ignore_white);

	static QList<QColor> AdjustPaletteValue(const QList<QColor>& palette);
private:
	QList<QColor> palette_;
};
