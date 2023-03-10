//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <stream/stream.h>
#include <QTimer>
#include <QFrame>

using xamp::stream::ComplexValarray;

class SpectrumWidget : public QFrame {
	Q_OBJECT
public:
	static constexpr auto kMaxBands = 128;
	static constexpr auto kFFTSize = 2048;

	explicit SpectrumWidget(QWidget* parent = nullptr);

	void Reset();

	void SetBarColor(QColor color);

public slots:
	void OnFftResultChanged(ComplexValarray const& result);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	void DrawBar(QPainter& painter, size_t num_bars);

	QColor bar_color_;
	ComplexValarray fft_result_;
	std::vector<float> bins_;
	std::vector<float> peak_delay_;
	QTimer timer_;
};
