#include <QPainter>
#include <QPaintEvent>
#include <qmath.h>

#include <widget/spectrograph.h>

Spectrograph::Spectrograph(QWidget* parent)
	: QFrame(parent)
	, low_freq_(0.0)
	, high_freq_(0.0)
	, frequency_(0.0)
	, bars_(256) {
	init();
}

void Spectrograph::setFrequency(double low_freq, double high_freq, double frequency) {
	low_freq_ = low_freq;
	high_freq_ = high_freq;
	frequency_ = frequency;
}

int Spectrograph::barIndex(qreal frequency) const {	
	const qreal bandWidth = (high_freq_ - low_freq_) / bars_.size();
	const int index = (frequency - low_freq_) / bandWidth;
	if (index < 0 || index >= bars_.size())
		Q_ASSERT(false);
	return index;
}

void Spectrograph::paintEvent(QPaintEvent* event) {
	QPainter painter(this);

	if (frequency_ == 0.0) {
		return;
	}

	QColor barColor(5, 184, 204);
	barColor = barColor.lighter();
	barColor.setAlphaF(0.75);

	const auto widgetWidth = this->width();
	const auto barPlusGapWidth = widgetWidth / bars_.size();
	const auto barWidth = 0.8 * barPlusGapWidth;
	const auto gapWidth = barPlusGapWidth - barWidth;
	const auto paddingWidth = widgetWidth - bars_.size() * (barWidth + gapWidth);
	const auto leftPaddingWidth = (paddingWidth + gapWidth) / 2;
	const auto barHeight = this->height() - 2 * gapWidth;

	const auto widget_rect = rect();

	for (int i = 0; i < results_.size(); ++i) {
		auto &b = bars_[barIndex(results_[i].frequency)];	
		b.amplitude = qMax(b.amplitude, results_[i].amplitude);
		b.value = (1.0 - b.amplitude) * barHeight;
		auto bar = widget_rect;
		bar.setLeft(rect().left() + leftPaddingWidth + (i * (gapWidth + barWidth)));
		bar.setWidth(barWidth);
		bar.setTop(rect().top() + gapWidth + b.value);
		bar.setBottom(rect().bottom() - gapWidth);
		painter.fillRect(bar, barColor);
	}
	event->accept();
}

void Spectrograph::init() {
	timer_.setTimerType(Qt::PreciseTimer);
	(void)QObject::connect(&timer_, &QTimer::timeout, [this]() {
		for (int i = 0; i < bars_.size(); ++i) {
			auto &b = bars_[i];
			if (b.amplitude >= 1.0) {
				b.amplitude -= 1.0;
			}
			else {
				b.amplitude = 0.0;
			}
		}
		update();
		});
}

void Spectrograph::onGetMagnitudeSpectrum(const std::vector<float>& mag) {
	static constexpr auto SpectrumAnalyserMultiplier{ 0.15 };

	results_.resize(mag.size());

	for (auto i = 0; i < mag.size(); ++i) {
		auto amplitude = SpectrumAnalyserMultiplier * qLn(mag[i]);
		amplitude = qMax(0.0, amplitude);
		amplitude = qMin(1.0, amplitude);
		auto db = 20.0 * std::log(mag[i]);
		auto frequency = double(i * frequency_) / (mag.size());
		results_[i] = SpectrumData{ frequency, amplitude, db };
	}
	
	update();
}

void Spectrograph::reset() {
	for (auto i = 0; i < results_.size(); ++i) {
		results_[i].amplitude = 0;
	}
	update();
}

void Spectrograph::start() {
	timer_.stop();
	timer_.setInterval(25);
	timer_.start();	
}

void Spectrograph::stop() {
	timer_.stop();
}