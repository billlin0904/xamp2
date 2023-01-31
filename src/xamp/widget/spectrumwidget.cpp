#include <QPainter>

#include <base/base.h>
#include <base/math.h>
#include <widget/widget_shared.h>
#include <stream/fft.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/actionmap.h>
#include <widget/spectrumwidget.h>

inline constexpr auto kMaxBands = 64;
inline constexpr auto kFFTSize = 64;

static double ToMag(const std::complex<float>& r) {
    return 10.0 * std::log10(std::pow(r.real(), 2) + std::pow(r.imag(), 2));
}

static double FreqToBin(int32_t freq, int32_t fft_size, double rate) {
    double ratio = static_cast<double>(fft_size * freq) / rate;
    return Round(ratio);
}

static double BinToFreq(int bin, int32_t fft_size, double rate) {
    return rate * bin / fft_size;
}

static void LinearInterpolation(double rate) {
    const auto kRoot24 = std::pow(2.0, (1.0 / 24.0));
    const auto kC0 = std::pow(440 * kRoot24,  -144);
    std::vector<std::tuple<double, double, double>> scale;

    scale.reserve(11 * 24);

    for (auto octave = 0; octave < 11; octave++) {
        for (auto note = 0; note < 24; note++) {
            const auto freq = std::pow(kC0 * kRoot24, (octave * 24 + note));
            const auto bin = FreqToBin(freq, kFFTSize, rate);
            const auto bin_freq  = BinToFreq(bin, kFFTSize, rate);
            const auto next_freq = BinToFreq(bin + 1, kFFTSize, rate);
            const auto ratio = (freq - bin_freq) / (next_freq - bin_freq);
            scale.push_back({freq, bin, ratio});
        }
    }
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

	setContextMenuPolicy(Qt::CustomContextMenu);
	(void)QObject::connect(this, &SpectrumWidget::customContextMenuRequested, [this](auto pt) {
		ActionMap<SpectrumWidget> action_map(this);

	(void)action_map.AddAction(tr("Bar style"), [this]() {
		setStyle(SpectrumStyles::BAR_STYLE);
	AppSettings::setEnumValue(kAppSettingSpectrumStyles, SpectrumStyles::BAR_STYLE);
		});

	(void)action_map.AddAction(tr("Wave style"), [this]() {
		setStyle(SpectrumStyles::WAVE_STYLE);
	AppSettings::setEnumValue(kAppSettingSpectrumStyles, SpectrumStyles::WAVE_STYLE);
		});

	(void)action_map.AddAction(tr("Wave line style"), [this]() {
		setStyle(SpectrumStyles::WAVE_LINE_STYLE);
	AppSettings::setEnumValue(kAppSettingSpectrumStyles, SpectrumStyles::WAVE_LINE_STYLE);
		});

	action_map.AddSeparator();

	(void)action_map.AddAction(tr("No Window"), []() {
		AppSettings::setEnumValue(kAppSettingWindowType, WindowType::NO_WINDOW);
		});

	(void)action_map.AddAction(tr("Hamming Window"), []() {
		AppSettings::setEnumValue(kAppSettingWindowType, WindowType::HAMMING);
		});

	(void)action_map.AddAction(tr("Blackman harris Window"), []() {
		AppSettings::setEnumValue(kAppSettingWindowType, WindowType::BLACKMAN_HARRIS);
		});

	action_map.exec(pt);
		});
}

void SpectrumWidget::onFFTResultChanged(ComplexValarray const& result) {
	auto max_bands = (std::min)(static_cast<size_t>(kMaxBands), result.size());

	if (mag_datas_.size() != max_bands) {
		mag_datas_.resize(max_bands);
		peak_delay_.resize(max_bands);
	}

	for (auto i = 0; i < max_bands; ++i) {
		mag_datas_[i] = ToMag(result[i]) * 0.025;
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

	QPainterPath path = generator_.GenerateSmoothCurve(points);
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
