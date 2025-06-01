//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QColor>

enum SpectrogramColor {
	SPECTROGRAM_COLOR_DEFAULT = 0,
	SPECTROGRAM_COLOR_SOX,
};

class ColorTable {
public:
	static constexpr double kMaxDb = 0;
	static constexpr double kMinDb = -120.0;
	static constexpr double kDbRange = kMaxDb - kMinDb;
	static constexpr size_t kLutSize = 1024;

	ColorTable();

	void setSpectrogramColor(SpectrogramColor color);

	QRgb operator[](double dB_val) const noexcept;

private:
	SpectrogramColor color_ = SpectrogramColor::SPECTROGRAM_COLOR_SOX;

	static QRgb danBrutonColor(double level) noexcept;

	static QRgb soxrColor(double level) noexcept;

	static const std::array<QRgb, kLutSize> kSoxrLut;
	static const std::array<QRgb, kLutSize> kDanBrutonLut;
	const QRgb* color_lut_ptr_;
};

