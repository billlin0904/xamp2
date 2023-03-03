//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <stream/stream.h>
#include <widget/smoothcurvegenerator2.h>
#include <QTimer>
#include <QFrame>

using xamp::stream::ComplexValarray;

enum SpectrumStyles {
	BAR_STYLE,
	WAVE_STYLE,
	WAVE_LINE_STYLE,
};

class SpectrumWidget : public QFrame {
	Q_OBJECT
public:
	explicit SpectrumWidget(QWidget* parent = nullptr);

	void reset();

	void setStyle(SpectrumStyles style);

	void setBarColor(QColor color);

public slots:
	void onFFTResultChanged(ComplexValarray const& result);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	void drawWave(QPainter& painter, size_t num_bars, bool is_line);

	void drawBar(QPainter& painter, size_t num_bars);

	SpectrumStyles style_{ BAR_STYLE };
	QColor bar_color_;
	ComplexValarray fft_result_;
	std::vector<float> mag_datas_;
	std::vector<float> peak_delay_;
	QTimer timer_;
	SmoothCurveGenerator2 generator_;
};
