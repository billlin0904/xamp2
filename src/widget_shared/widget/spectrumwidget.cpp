#include <widget/spectrumwidget.h>

#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <widget/appsettings.h>
#include <widget/actionmap.h>

SpectrumWidget::SpectrumWidget(QWidget* parent)
	: QFrame(parent)
	, buffer_(kBufferSize, std::valarray<float>(kMaxBands)) {
	(void) QObject::connect(&timer_, &QTimer::timeout, [this]() {
		update();
	});

	timer_.setTimerType(Qt::PreciseTimer);
	timer_.start(15);
}

void SpectrumWidget::setBarColor(const QColor& color) {
	bar_color_ = color;
}

void SpectrumWidget::start() {
	is_stop_ = false;
}

void SpectrumWidget::onFftResultChanged(const ComplexValarray& fft_data) {
	if (is_stop_) {
		return;
	}
	fft_data_ = fft_data;
}

void SpectrumWidget::onThemeChangedFinished(ThemeColor theme_color) {
	setBarColor(qTheme.highlightColor());
}

void SpectrumWidget::paintEvent(QPaintEvent* /*event*/) {
	QPainter painter(this);

	painter.setRenderHints(QPainter::Antialiasing, true);
	painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
	painter.setRenderHints(QPainter::TextAntialiasing, true);

	if (fft_data_.size() == 0) {
		return;
	}

	const int samples_per_band = static_cast<int>(fft_data_.size() / kMaxBands);

	std::valarray<float> band_energies(kMaxBands);
	for (int i = 0; i < kMaxBands; i++) {
		const int start_index = i * samples_per_band;
		const int end_index = (i + 1) * samples_per_band;

		// 計算當前頻帶的能量值
		float band_energy = 0.0f;
		for (int j = start_index; j < end_index; j++) {
			band_energy += std::norm(fft_data_[j]);
		}
		const float band_power = band_energy / static_cast<float>(samples_per_band);
		band_energies[i] = band_power;
	}

	// 計算每個頻帶的能量值的db值
	const std::valarray<float> band_db_values = 10.0f * std::log10(band_energies);
	if ((band_db_values.max)() == -std::numeric_limits<float>::infinity()) {
		return;
	}

	// 將頻譜資料保存到緩衝區
	buffer_[buffer_ptr_] = band_db_values;
	// 更新緩衝區指標
	buffer_ptr_ = (buffer_ptr_ + 1) % kBufferSize;

	std::valarray<float> average_spectrum(kMaxBands);
	for (int i = 0; i < kBufferSize; i++) {
		average_spectrum += buffer_[i];
	}
	average_spectrum /= kBufferSize;

	const float max_db_value = (average_spectrum.max)();

	const float rect_width = static_cast<float>(width()) / static_cast<float>(kMaxBands);
	const auto rect_height = static_cast<float>(height());

	QVector<QColor> colors;

	constexpr float kMinRectHeight = 1.0f;

	if (style_ == SpectrumStyles::BAR_STYLE) {
		QVector<QRectF> rects;
		for (int i = 0; i < kMaxBands; i++) {
			const float db_value = average_spectrum[i];
			const float db_normalized = (std::min)(db_value / std::abs(max_db_value), 1.0f);
			const float rect_x = static_cast<float>(i) * rect_width;
			float rect_y = rect_height * (1.0f - db_normalized);
			if (rect_y < kMinRectHeight) {
				rect_y = kMinRectHeight;
			}
			QRectF rect(rect_x, rect_y, rect_width, rect_height - rect_y);
			rects.append(rect);
			colors.append(QColor::fromHsvF(static_cast<float>(i) / static_cast<float>(kMaxBands), 1.0f, 1.0f));
		}

		for (int i = 0; i < kMaxBands; i++) {
			painter.fillRect(rects[i], colors[i]);
		}
	} else {		
		float max_db_value = average_spectrum[0];
		int max_index = 0;

		for (int i = 1; i < kMaxBands; ++i) {
			if (average_spectrum[i] > max_db_value) {
				max_db_value = average_spectrum[i];
				max_index = i;
			}
		}

		QPainterPath path;
		float highest_point = rect_height;

		path.moveTo(0, height());
		for (int i = 0; i < kMaxBands; i++) {
			const float db_value = average_spectrum[i];
			const float db_normalized = (std::min)(db_value / std::abs(max_db_value), 1.0f);
			const float rect_x = static_cast<float>(i) * rect_width;
			float rect_y = rect_height * (1.0f - db_normalized);
			if (rect_y < kMinRectHeight) {
				rect_y = kMinRectHeight;
			}
			if (rect_y < highest_point) {
				highest_point = rect_y;
			}
			path.lineTo(rect_x, rect_y);
		}
		
		path.lineTo(width(), height());
		path.closeSubpath();

		QLinearGradient newGradient(0, highest_point, 0, rect_height);
		newGradient.setColorAt(1.0, bar_color_);
		painter.fillPath(path, newGradient);
	}
}

void SpectrumWidget::reset() {
	fft_data_ = 0;
	for (auto &buffer : buffer_) {
		buffer = 0;
	}
	is_stop_ = true;
	update();
}
