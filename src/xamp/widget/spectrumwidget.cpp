#include <QPainter>

#include <base/base.h>
#include <base/math.h>

#include <widget/appsettings.h>
#include <widget/spectrumwidget.h>

inline constexpr auto kMaxBands = 256;

using xamp::base::toMag;

SpectrumWidget::SpectrumWidget(QWidget* parent)
	: QFrame(parent) {
	(void) QObject::connect(&timer_, &QTimer::timeout, [this]() {
		for (auto &peek : peak_delay_) {
			peek -= 0.05f;
		}
		update();
		});
	timer_.setTimerType(Qt::PreciseTimer);
	timer_.start(25);
	bar_color_ = QColor(5, 184, 204);
}

void SpectrumWidget::onFFTResultChanged(ComplexValarray const& result) {
	auto max_bands = (std::min)(static_cast<size_t>(kMaxBands), result.size());

	if (mag_datas_.size() != max_bands) {
		mag_datas_.resize(max_bands);
		peak_delay_.resize(max_bands);
	}

	for (auto i = 0; i < max_bands; ++i) {
		mag_datas_[i] = toMag(result[i]) * 0.025;
		//mag_datas_[i] = toMag(result[i]) / max_bands;
		mag_datas_[i] = (std::max)(mag_datas_[i], 0.01f);
		mag_datas_[i] = (std::min)(mag_datas_[i], 0.9f);
		if (peak_delay_[i] <= mag_datas_[i]) {
			peak_delay_[i] = mag_datas_[i];
		}
	}
}

void SpectrumWidget::drawBar(QPainter &painter, size_t num_bars) {
	const auto widget_width = width();
	const auto bar_plus_gap_width = widget_width / num_bars;
	const auto bar_width = 0.8f * bar_plus_gap_width;
	const auto gap_width = bar_plus_gap_width - bar_width;
	const auto padding_width = widget_width - num_bars * (bar_width + gap_width);
	const auto left_padding_width = (padding_width + gap_width) / 2;
	const auto bar_height = height() - 2 * gap_width;

	for (auto i = 0; i < num_bars; ++i) {
		const auto value = peak_delay_[i];
		QRect bar = rect();
		bar.setLeft(rect().left() + left_padding_width + (i * (gap_width + bar_width)));
		bar.setWidth(bar_width);
		bar.setTop(rect().top() + gap_width + (1.0f - value) * bar_height);
		bar.setBottom(rect().bottom() - gap_width);
		painter.fillRect(bar, bar_color_);
	}
}

void SpectrumWidget::drawWave(QPainter& painter, size_t num_bars, bool is_line) {
	float max = peak_delay_.at(0);
	float min = peak_delay_.at(0);
	for (size_t i = 1; i < num_bars; i++) {
		if (max < peak_delay_.at(i)) {
			max = peak_delay_.at(i);
		}

		if (min > peak_delay_.at(i)) {
			min = peak_delay_.at(i);
		}
	}

	std::vector<QPointF> points;
	points.reserve(num_bars + 2);
	points.push_back(QPoint(0, height()));
	for (auto i = 0; i < num_bars; i++) {
		double x = i * width() / num_bars;
		double y = height() - (((peak_delay_.at(i) - min) / (max - min)) * height());
		points.push_back(QPointF(x, y));
	}
	points.push_back(QPoint(width(), height()));

	QPainterPath path = generator_.generateSmoothCurve(points);
	path.closeSubpath();
	if (is_line) {
		painter.setPen(bar_color_);
		painter.drawPath(path);
	} else {
		/*QLinearGradient gradient(QPoint(width(), 0), QPoint(width(), height()));
		gradient.setColorAt(0.0, bar_color);
		gradient.setColorAt(1.0, QColor(0, 0, 0, 0));*/
		painter.fillPath(path, bar_color_);
	}
}

void SpectrumWidget::paintEvent(QPaintEvent* /*event*/) {
	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing, true);

	const auto num_bars = peak_delay_.size();
	if (!num_bars) {
		return;
	}

	switch (style_) {
	case SpectrumStyles::BAR_STYLE:
		drawBar(painter, num_bars);
		break;
	case SpectrumStyles::WAVE_STYLE:
		drawWave(painter, num_bars, false);
		break;
	case SpectrumStyles::WAVE_LINE_STYLE:
		drawWave(painter, num_bars, true);
		break;
	}
}

void SpectrumWidget::setBarColor(QColor color) {
	bar_color_ = color;
}

void SpectrumWidget::setStyle(SpectrumStyles style) {
	style_ = style;
}

void SpectrumWidget::reset() {
	mag_datas_.clear();
	peak_delay_.clear();
	update();
}