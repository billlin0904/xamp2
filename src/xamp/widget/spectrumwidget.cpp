#include <QPainter>
#include <QPainterPath>
#include <base/base.h>
#include <widget/actionmap.h>
#include <widget/spectrumwidget.h>

inline constexpr float kDecay = 0.05f;
inline constexpr auto kMaxBands = 120;

class SmoothCurveGenerator2 {
public:
	static QPainterPath generateSmoothCurve(const QVector<QPointF>& points);

	static void calculateFirstControlPoints(double*& result, const double* rhs, int n);

	static void calculateControlPoints(const QVector<QPointF>& knots,
		QVector<QPointF>* firstControlPoints,
		QVector<QPointF>* secondControlPoints);
};

QPainterPath SmoothCurveGenerator2::generateSmoothCurve(const QVector<QPointF>& points) {
	QPainterPath path;
	int len = points.size();

	if (len < 2) {
		return path;
	}

	QVector<QPointF> firstControlPoints;
	QVector<QPointF> secondControlPoints;
	calculateControlPoints(points, &firstControlPoints, &secondControlPoints);

	path.moveTo(points[0].x(), points[0].y());

	// Using bezier curve to generate a smooth curve.
	for (int i = 0; i < len - 1; ++i) {
		path.cubicTo(firstControlPoints[i], secondControlPoints[i], points[i + 1]);
	}

	return path;
}

void SmoothCurveGenerator2::calculateFirstControlPoints(double*& result, const double* rhs, int n) {
	result = new double[n];
	double* tmp = new double[n];
	double b = 2.0;
	result[0] = rhs[0] / b;

	// Decomposition and forward substitution.
	for (int i = 1; i < n; i++) {
		tmp[i] = 1 / b;
		b = (i < n - 1 ? 4.0 : 3.5) - tmp[i];
		result[i] = (rhs[i] - result[i - 1]) / b;
	}

	for (int i = 1; i < n; i++) {
		result[n - i - 1] -= tmp[n - i] * result[n - i]; // Backsubstitution.
	}

	delete[] tmp;
}

void SmoothCurveGenerator2::calculateControlPoints(const QVector<QPointF>& knots,
	QVector<QPointF>* firstControlPoints,
	QVector<QPointF>* secondControlPoints) {
	int n = knots.size() - 1;

	for (int i = 0; i < n; ++i) {
		firstControlPoints->append(QPointF());
		secondControlPoints->append(QPointF());
	}

	if (n == 1) {
		// Special case: Bezier curve should be a straight line.
		// P1 = (2P0 + P3) / 3
		(*firstControlPoints)[0].rx() = (2 * knots[0].x() + knots[1].x()) / 3;
		(*firstControlPoints)[0].ry() = (2 * knots[0].y() + knots[1].y()) / 3;

		// P2 = 2P1 ¡V P0
		(*secondControlPoints)[0].rx() = 2 * (*firstControlPoints)[0].x() - knots[0].x();
		(*secondControlPoints)[0].ry() = 2 * (*firstControlPoints)[0].y() - knots[0].y();

		return;
	}

	// Calculate first Bezier control points
	double* xs = 0;
	double* ys = 0;
	double* rhsx = new double[n]; // Right hand side vector
	double* rhsy = new double[n]; // Right hand side vector

	// Set right hand side values
	for (int i = 1; i < n - 1; ++i) {
		rhsx[i] = 4 * knots[i].x() + 2 * knots[i + 1].x();
		rhsy[i] = 4 * knots[i].y() + 2 * knots[i + 1].y();
	}
	rhsx[0] = knots[0].x() + 2 * knots[1].x();
	rhsx[n - 1] = (8 * knots[n - 1].x() + knots[n].x()) / 2.0;
	rhsy[0] = knots[0].y() + 2 * knots[1].y();
	rhsy[n - 1] = (8 * knots[n - 1].y() + knots[n].y()) / 2.0;

	// Calculate first control points coordinates
	calculateFirstControlPoints(xs, rhsx, n);
	calculateFirstControlPoints(ys, rhsy, n);

	// Fill output control points.
	for (int i = 0; i < n; ++i) {
		(*firstControlPoints)[i].rx() = xs[i];
		(*firstControlPoints)[i].ry() = ys[i];

		if (i < n - 1) {
			(*secondControlPoints)[i].rx() = 2 * knots[i + 1].x() - xs[i + 1];
			(*secondControlPoints)[i].ry() = 2 * knots[i + 1].y() - ys[i + 1];
		}
		else {
			(*secondControlPoints)[i].rx() = (knots[n].x() + xs[n - 1]) / 2;
			(*secondControlPoints)[i].ry() = (knots[n].y() + ys[n - 1]) / 2;
		}
	}

	delete xs;
	delete ys;
	delete[] rhsx;
	delete[] rhsy;
}

SpectrumWidget::SpectrumWidget(QWidget* parent)
	: QFrame(parent) {

	setContextMenuPolicy(Qt::CustomContextMenu);
	(void)QObject::connect(this, &SpectrumWidget::customContextMenuRequested, [this](auto pt) {
		ActionMap<SpectrumWidget> action_map(this);

		(void)action_map.addAction(tr("Bar style"), [this]() {
			setStyle(SpectrumStyles::BAR_STYLE);
			});

		(void)action_map.addAction(tr("Wave style"), [this]() {
			setStyle(SpectrumStyles::WAVE_STYLE);
			});

		(void)action_map.addAction(tr("Wave line style"), [this]() {
			setStyle(SpectrumStyles::WAVE_LINE_STYLE);
			});

		action_map.exec(pt);
	});
}

void SpectrumWidget::onFFTResultChanged(ComplexValarray const& result) {
	const auto frame_size = result.size();
	if (freq_data_.size() != frame_size) {
		multiplier_ = static_cast<float>((frame_size / 2.0) / kMaxBands);
		freq_data_.resize(frame_size);
		bars_.resize(kMaxBands);
	}

	for (auto i = 0; i < result.size(); ++i) {
		freq_data_[i] =
			std::hypot(result[i].imag(), result[i].real()) * 0.03;
		freq_data_[i] = (std::max)(freq_data_[i], 0.0f);
		freq_data_[i] = (std::min)(freq_data_[i], 0.8f);
	}
	update();
}

void SpectrumWidget::drawBar(QPainter &painter, size_t num_bars) {
	QColor bar_color(5, 184, 204);

	const auto widget_width = width();
	const auto bar_plus_gap_width = widget_width / num_bars;
	const auto bar_width = 0.8f * bar_plus_gap_width;
	const auto gap_width = bar_plus_gap_width - bar_width;
	const auto padding_width = widget_width - num_bars * (bar_width + gap_width);
	const auto left_padding_width = (padding_width + gap_width) / 2;
	const auto bar_height = height() - 2 * gap_width;

	for (auto i = 0; i < num_bars; ++i) {
		const auto value = freq_data_[i];
		QRect bar = rect();
		bar.setLeft(rect().left() + left_padding_width + (i * (gap_width + bar_width)));
		bar.setWidth(bar_width);
		bar.setTop(rect().top() + gap_width + (1.0f - value) * bar_height);
		bar.setBottom(rect().bottom() - gap_width);
		painter.fillRect(bar, bar_color);
	}
}

void SpectrumWidget::drawWave(QPainter& painter, size_t num_bars, bool is_line) {
	QColor bar_color(5, 184, 204);

	float max = freq_data_.at(0);
	float min = freq_data_.at(0);
	for (size_t i = 1; i < num_bars; i++) {
		if (max < freq_data_.at(i)) {
			max = freq_data_.at(i);
		}

		if (min > freq_data_.at(i)) {
			min = freq_data_.at(i);
		}
	}

	QVector<QPointF> points;
	points.append(QPoint(0, height()));
	for (auto i = 0; i < num_bars; i++) {
		double x = i * width() / num_bars;
		double y = height() - (((freq_data_.at(i) - min) / (max - min)) * height());
		points.append(QPointF(x, y));
	}
	points.append(QPoint(width(), height()));

	QPainterPath path = SmoothCurveGenerator2::generateSmoothCurve(points);
	path.closeSubpath();
	if (is_line) {
		painter.setPen(bar_color);
		painter.drawPath(path);
	} else {
		painter.fillPath(path, bar_color);
	}
}

void SpectrumWidget::paintEvent(QPaintEvent* /*event*/) {
	QPainter painter(this);

	painter.setRenderHints(QPainter::Antialiasing, true);

	QColor bar_color(5, 184, 204);
	bar_color = bar_color.lighter();
	bar_color.setAlphaF(0.75);

	const auto num_bars = bars_.size();
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

void SpectrumWidget::setStyle(SpectrumStyles style) {
	style_ = style;
}

void SpectrumWidget::reset() {
	
}