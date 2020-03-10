#include <QPainter>
#include <QPaintEvent>
#include <qmath.h>

#include <widget/spectrograph.h>

Spectrograph::Spectrograph(QWidget* parent)
	: QFrame(parent)
	, low_freq_(0.0)
	, high_freq_(0.0)
	, frequency_(0.0)
	, bars_(128) {
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

	const int widgetWidth = this->width();
	const int barPlusGapWidth = widgetWidth / bars_.size();
	const int barWidth = 0.8 * barPlusGapWidth;
	const int gapWidth = barPlusGapWidth - barWidth;
	const int paddingWidth = widgetWidth - bars_.size() * (barWidth + gapWidth);
	const int leftPaddingWidth = (paddingWidth + gapWidth) / 2;
	const int barHeight = this->height() - 2 * gapWidth;

	for (int i = 0; i < spectrum_.size(); ++i) {
		auto b = bars_[barIndex(spectrum_[i].frequency)];
		b.value = qMax(b.value, spectrum_[i].amplitude);		
		const double value = b.value;
		auto bar = rect();
		bar.setLeft(rect().left() + leftPaddingWidth + (i * (gapWidth + barWidth)));
		bar.setWidth(barWidth);
		bar.setTop(rect().top() + gapWidth + (1.0 - value) * barHeight);
		bar.setBottom(rect().bottom() - gapWidth);
		painter.fillRect(bar, barColor);
	}
	event->accept();
}

void Spectrograph::onGetMagnitudeSpectrum(const std::vector<float>& magnitude) {	
	constexpr auto SpectrumAnalyserMultiplier{ 0.15 };

	spectrum_.resize(magnitude.size());

	for (auto i = 0; i < magnitude.size(); ++i) {		
		qreal amplitude = SpectrumAnalyserMultiplier * qLn(magnitude[i]);
		amplitude = qMax(qreal(0.0), amplitude);
		amplitude = qMin(qreal(1.0), amplitude);
		auto frequency = qreal(i * frequency_) / (magnitude.size());
		spectrum_[i] = Spectrum{ frequency, amplitude };
	}
	
	update();
}

void Spectrograph::reset() {

}