#include <cmath>
#include <widget/colorpicker.h>

class BilateralFilter {
public:
	BilateralFilter(int32_t radius, double sigmaD, double sigmaC)
		: radius_(radius)
		, sigmaD_(sigmaD)
		, sigmaC_(sigmaC) {
		auto size = 2 * radius + 1;
		dw_.resize(size);
		for (auto& table : dw_) {
			table.resize(size);
		}
		cw_.resize(256);
		BuildDistanceWeightTable();
		BuildDistanceWeightTable();
	}

	void Apply(QImage& image) const {
		for (auto row = radius_; row < image.width() - radius_; row++) {
			for (auto col = radius_; col < image.height() - radius_; col++) {
				float weightSum[3] = { 0 };
				float pixelSum[3] = { 0 };
				
				auto cur = image.pixelColor(row, col);

				for (auto i = -radius_; i <= radius_; i++) {
					for (auto j = -radius_; j <= radius_; j++) {
						auto neighbor = image.pixelColor(row + i, col + j);

						auto distance_weight = dw_[i + radius_][j + radius_];

						float red_weight = distance_weight * cw_[abs(neighbor.red() - cur.red())];
						pixelSum[0] += neighbor.red() * red_weight;
						weightSum[0] += red_weight;

						float green_weight = distance_weight * cw_[abs(neighbor.green() - cur.green())];
						pixelSum[1] += neighbor.green() * green_weight;
						weightSum[1] += green_weight;

						float blue_weight = distance_weight * cw_[abs(neighbor.blue() - cur.blue())];
						pixelSum[2] += neighbor.blue() * blue_weight;
						weightSum[2] += blue_weight;
					}
				}

				cur.setRed(pixelSum[0] / weightSum[0]);
				cur.setGreen(pixelSum[1] / weightSum[1]);
				cur.setBlue(pixelSum[2] / weightSum[2]);

				image.setPixelColor(row, col, cur);
			}
		}
	}

private:
	void BuildDistanceWeightTable() {
		for (auto i = -radius_; i <= radius_; i++) {
			for (size_t j = -radius_; j <= radius_; j++) {
				dw_[i + radius_][j + radius_]
					= exp(-0.5 * (i * i + j * j) / (sigmaD_ * sigmaD_));
			}
		}
	}

	void BuildColorWeightTable() {
		for (auto i = 0; i < cw_.size(); i++) {
			cw_[i] = exp(-0.5 * (i * i) / (sigmaC_ * sigmaC_));
		}
	}

	int32_t radius_;
	double sigmaD_;
	double sigmaC_;
	std::vector<std::vector<double>> dw_;
	std::vector<double> cw_;
};

static bool isBlackOrWhite(const QColor& color) noexcept {
	const auto r = double(color.red()) / 255;
	const auto g = double(color.green()) / 255;
	const auto b = double(color.blue()) / 255;

	return ((r > 0.91 && g > 0.91 && b > 0.91) || (r < 0.09 && g < 0.09 && b < 0.09));
}

static bool isDarkColor(const QColor& color) noexcept {
	const auto r = double(color.red()) / 255;
	const auto g = double(color.green()) / 255;
	const auto b = double(color.blue()) / 255;
	const auto lum = 0.2126 * r + 0.7152 * g + 0.0722 * b;
	return lum < 0.5;
}

static QColor colorWithMinimumSaturation(const QColor& color, const float min_saturation) noexcept {
	auto const hsv = color.toHsv();

	if (hsv.saturation() < min_saturation) {
		QColor temp(QColor::Hsv);
		temp.setHsl(hsv.hue(), min_saturation, hsv.value());
		return temp.toRgb();
	}
	return color;
}

static bool isContrastingColor(const QColor& background_color, const QColor& foreground_color) noexcept {
	const auto br = double(background_color.red()) / 255;
	const auto bg = double(background_color.green()) / 255;
	const auto bb = double(background_color.blue()) / 255;

	const auto fr = double(foreground_color.red()) / 255;
	const auto fg = double(foreground_color.green()) / 255;
	const auto fb = double(foreground_color.blue()) / 255;

	const auto b_lum = 0.2126 * br + 0.7152 * bg + 0.0722 * bb;
	const auto f_lum = 0.2126 * fr + 0.7152 * fg + 0.0722 * fb;

	double contrast;

	if (b_lum > f_lum) {
		contrast = (b_lum + 0.05) / (f_lum + 0.05);
	}
	else {
		contrast = (f_lum + 0.05) / (b_lum + 0.05);
	}
	return contrast > 1.6;
}

static bool isDistinctColor(const QColor& color_a, const QColor& color_b) noexcept {
	const auto r = double(color_a.red()) / 255;
	const auto g = double(color_a.green()) / 255;
	const auto b = double(color_a.blue()) / 255;
	const auto a = double(color_a.alpha()) / 255;

	const auto r1 = double(color_b.red()) / 255;
	const auto g1 = double(color_b.green()) / 255;
	const auto b1 = double(color_b.blue()) / 255;
	const auto a1 = double(color_b.alpha()) / 255;

	const auto threshold = 0.25;

	if (abs(r - r1) > threshold
		|| abs(g - g1) > threshold
		|| abs(b - b1) > threshold
		|| abs(a - a1) > threshold) {
		return !(abs(r - g) < 0.03 && abs(r - b) < 0.03 && (abs(r1 - g1) < 0.03 && abs(r1 - b1) < 0.03));
	}
	return false;
}

constexpr double COLOR_THRESHOLD_MINIMUM_PERCENTAGE = 0.01;
constexpr double EDGE_COLOR_DISCARD_THRESHOLD = 0.3;
constexpr float MINIMUM_SATURATION_THRESHOLD = 0.15f;

ColorPicker::ColorPicker() {
}

void ColorPicker::loadImage(const QPixmap& image) {
	loadImage(image.toImage());
}

QImage ColorPicker::getTestImage(const QImage& image) const {
	static const BilateralFilter filter{ 36, 0.1, 0.1 };
	constexpr QSize image_size{ 36, 36 };
	
	auto small_size = image.scaled(image_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	filter.Apply(small_size);
	return small_size;
}

void ColorPicker::loadImage(const QImage& image) {
	colors_.clear();

	auto test_image = getTestImage(image);

	for (auto x = 0; x < test_image.width(); ++x) {
		for (auto y = 0; y < test_image.height(); ++y) {
			++colors_[test_image.pixelColor(x, y)];
		}
	}

	background_color_ = findEdgeColor();
	findTextColors();
}

void ColorPicker::findTextColors() noexcept {
	ColorSet sorted_colors;

	const auto find_dark_text_color = !isDarkColor(background_color_);
	for (const auto& color : colors_) {
		const auto current_color = colorWithMinimumSaturation(color.first,
			MINIMUM_SATURATION_THRESHOLD);
		if (isDarkColor(current_color) == find_dark_text_color) {
			sorted_colors[color.first] += color.second;
		}
	}

	for (const auto& counted_color : sorted_colors) {
		const auto color = counted_color.first;

		if (!primary_color_.isValid()) {
			if (isContrastingColor(color, background_color_)) {
				primary_color_ = color;
			}
		}
		else if (!secondary_color_.isValid()) {
			if (!isDistinctColor(primary_color_, color) ||
				!isContrastingColor(color, background_color_)) {
				continue;
			}
			secondary_color_ = color;
		}
		else if (!detail_color_.isValid()) {
			if (!isDistinctColor(secondary_color_, color) ||
				!isDistinctColor(primary_color_, color) ||
				!isContrastingColor(color, background_color_)) {
				continue;
			}
			detail_color_ = color;
			break;
		}
	}
}

QColor ColorPicker::findEdgeColor() const noexcept {
	auto itr = colors_.cbegin();
	if (itr == colors_.end()) {
		return Qt::black;
	}

	std::map<int32_t, QColor> counted_color;
	for (auto pair : colors_) {
		counted_color[pair.second] = pair.first;
	}

	std::pair<QColor, int32_t> proposed_edge_color(counted_color.rbegin()->second, counted_color.rbegin()->first);

	for (; itr != colors_.cend(); ++itr) {
		const auto edge_color_ratio = static_cast<double>((*itr).second) / proposed_edge_color.second;

		if (edge_color_ratio <= EDGE_COLOR_DISCARD_THRESHOLD) {
			break;
		}

		if (!isBlackOrWhite((*itr).first)) {
			proposed_edge_color = (*itr);
			break;
		}
	}
	return proposed_edge_color.first;
}
