//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <stream/stream.h>

#include <QTimer>
#include <QFrame>

#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>

using xamp::stream::ComplexValarray;

enum SpectrumStyles {
	BAR_STYLE,
	WAVE_STYLE,
};

class XAMP_WIDGET_SHARED_EXPORT SpectrumWidget : public QFrame {
	Q_OBJECT
public:
	static constexpr auto kMaxBands = 160;
	static constexpr auto kBufferSize = 5;

	explicit SpectrumWidget(QWidget* parent = nullptr);

	void setBarColor(const QColor& color);

	void start();

	void reset();

public slots:
	void onFftResultChanged(const ComplexValarray& fft_data);

	void onThemeChangedFinished(ThemeColor theme_color);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	bool is_stop_{ false };
	int32_t buffer_ptr_{0};
	SpectrumStyles style_{ SpectrumStyles::BAR_STYLE };
	QColor bar_color_;
	ComplexValarray fft_data_;
	std::vector<std::valarray<float>> buffer_;
	QTimer timer_;
};
