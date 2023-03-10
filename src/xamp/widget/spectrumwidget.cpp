#include <QPainter>

#include <base/math.h>
#include <widget/widget_shared.h>
#include <stream/fft.h>

#include <widget/appsettings.h>
#include <widget/actionmap.h>
#include <widget/spectrumwidget.h>

static double ToMag(const std::complex<float>& r) {
    return 10.0 * std::log10(std::pow(r.real(), 2) + std::pow(r.imag(), 2));
}

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

void SpectrumWidget::OnFftResultChanged(ComplexValarray const& result) {
	const auto max_bands = (std::min)(static_cast<size_t>(kMaxBands), 
	                                  result.size());

	if (bins_.size() != max_bands) {
		bins_.resize(max_bands);
		peak_delay_.resize(max_bands);
	}

	for (auto i = 0; i < max_bands; ++i) {
		if (result[i].imag() == 0 && result[i].real() == 0) {
			bins_[i] = 0;
		} else {
			bins_[i] = ToMag(result[i]);
		}		
		peak_delay_[i] = std::abs(bins_[i]) / std::sqrt(2) / 2;
		peak_delay_[i] = (std::min)(peak_delay_[i], 2048.0f);
		peak_delay_[i] = peak_delay_[i] / 2048.0f * 128;
	}
}

void SpectrumWidget::DrawBar(QPainter &painter, size_t num_bars) {
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

void SpectrumWidget::paintEvent(QPaintEvent* /*event*/) {
	QPainter painter(this);

	painter.setRenderHints(QPainter::Antialiasing, true);
	painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
	painter.setRenderHints(QPainter::TextAntialiasing, true);

	const auto num_bars = peak_delay_.size();
	if (!num_bars) {
		return;
	}

	DrawBar(painter, num_bars);
}

void SpectrumWidget::SetBarColor(QColor color) {
	bar_color_ = color;
}

void SpectrumWidget::Reset() {
	bins_.clear();
	peak_delay_.clear();
	update();
}
