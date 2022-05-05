#include <QPainter>
#include <widget/spectrumwidget.h>

SpectrumWidget::SpectrumWidget(QWidget* parent)
	: QFrame(parent) {
}

void SpectrumWidget::setParams(const double& low_freq, const double& high_freq) {
	low_freq_ = low_freq;
	high_freq_ = high_freq;
}

void SpectrumWidget::onFFTResultChanged(ComplexValarray const& result) {
	freq_data_.resize(result.size());
	bars_.resize(128);

	for (auto i = 0; i < result.size(); ++i) {
		freq_data_[i] =
			std::hypot(result[i].imag(), result[i].real()) * 0.03;
	}
	update();
}

void SpectrumWidget::paintEvent(QPaintEvent* /*event*/) {
	QPainter painter(this);

	QColor bar_color(5, 184, 204);
	bar_color = bar_color.lighter();
	bar_color.setAlphaF(0.75);

	const auto num_bars = bars_.size();
	if (!num_bars) {
		return;
	}

	const auto widget_width = width();
	const auto bar_plus_gap_width = widget_width / num_bars;
	const auto bar_width = 0.8 * bar_plus_gap_width;
	const auto gap_width = bar_plus_gap_width - bar_width;
	const auto padding_width = widget_width - num_bars * (bar_width + gap_width);
	const auto left_padding_width = (padding_width + gap_width) / 2;
	const auto bar_height = height() - 2 * gap_width;

	for (auto i = 0; i < num_bars; ++i) {
		double value = freq_data_[i];
		if (value > 1.0) {
			value = 1.0;
		}
		QRect bar = rect();
		bar.setLeft(rect().left() + left_padding_width + (i * (gap_width + bar_width)));
		bar.setWidth(bar_width);
		bar.setTop(rect().top() + gap_width + (1.0 - value) * bar_height);
		bar.setBottom(rect().bottom() - gap_width);
		painter.fillRect(bar, bar_color);
	}
}

void SpectrumWidget::reset() {
	
}