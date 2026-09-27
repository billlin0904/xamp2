//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
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

	inline QRgb operator[](double dB_val) const {
		dB_val = std::clamp(dB_val, kMinDb, kMaxDb);
		const double ratio = (dB_val - kMinDb) / kDbRange;
		const size_t idx = static_cast<size_t>(ratio * (kLutSize - 1));
		return color_lut_ptr_[idx];
	}

private:
	SpectrogramColor color_ = SpectrogramColor::SPECTROGRAM_COLOR_SOX;

	static QRgb danBrutonColor(double level) ;

	static QRgb soxrColor(double level) ;

	static const std::array<QRgb, kLutSize> kSoxrLut;
	static const std::array<QRgb, kLutSize> kDanBrutonLut;
	const QRgb* color_lut_ptr_;
};

