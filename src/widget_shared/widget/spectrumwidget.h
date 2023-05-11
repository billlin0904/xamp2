//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <stream/stream.h>
#include <QTimer>
#include <QFrame>

#include <widget/widget_shared_global.h>

using xamp::stream::ComplexValarray;

enum SpectrumStyles {
	BAR_STYLE,
	WAVE_STYLE,
};

class XAMP_WIDGET_SHARED_EXPORT SpectrumWidget : public QFrame {
	Q_OBJECT
public:
	static constexpr auto kMaxBands = 256;
	static constexpr auto kBufferSize = 5;

	explicit SpectrumWidget(QWidget* parent = nullptr);

	void SetSampleRate(int32_t sample_rate);

	void SetFftSize(int32_t fft_size);

	void Reset();

public slots:
	void OnFftResultChanged(ComplexValarray const& result);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	int32_t buffer_ptr_{0};
	int32_t sample_rate_{44100};
	int32_t fft_size_{4096};
	SpectrumStyles style_{ SpectrumStyles::WAVE_STYLE };
	ComplexValarray fft_data_;
	std::vector<std::valarray<float>> buffer_;
	QTimer timer_;
};
