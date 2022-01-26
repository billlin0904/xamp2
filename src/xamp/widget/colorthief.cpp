#include <algorithm>
#include <tuple>
#include <vector>
#include <optional>

#include <base/stl.h>

#include <QPixmap>
#include <widget/colorthief.h>

using Color = std::tuple<uint8_t, uint8_t, uint8_t>;

static constexpr double kFractByPopulations = 0.75;
static constexpr int32_t kMaxIteration = 1000;

class ColorStatis final {
public:
	static constexpr int32_t SIGBITS = 5;
	static constexpr int32_t RSHIFT = 8 - SIGBITS;

	ColorStatis(int32_t red_min, int32_t rmax, int32_t gmin, int32_t gmax, int32_t bmin, int32_t bmax,
		const std::vector<int32_t>& histo)
		: red_min(red_min)
		, red_max(rmax)
		, green_min(gmin)
		, green_max(gmax)
		, blue_min(bmin)
		, blue_max(bmax)
		, count_(0)
		, histo_(histo) {
		CountColor();
		AvgColor();
	}

	bool Contains(const Color& pixel) const noexcept {
		const auto rval = std::get<0>(pixel) >> RSHIFT;
		const auto gval = std::get<1>(pixel) >> RSHIFT;
		const auto bval = std::get<2>(pixel) >> RSHIFT;
		return rval >= red_min && rval <= red_max &&
			gval >= green_min && gval <= green_max &&
			bval >= blue_min && bval <= blue_max;
	}

	void CountColor() noexcept {
		auto npix = 0;
		for (auto i = red_min; i < red_max + 1; i++) {
			for (auto j = green_min; j < green_max + 1; j++) {
				for (auto k = blue_min; k < blue_max + 1; k++) {
					const auto index = GetColorIndex(i, j, k);
					npix += histo_[index];
				}
			}
		}
		count_ = npix;
	}

	void AvgColor() noexcept {
		auto ntot = 0;
		auto mult = 1 << (8 - SIGBITS);
		auto r_sum = 0.0;
		auto g_sum = 0.0;
		auto b_sum = 0.0;

		for (auto i = red_min; i < red_max + 1; i++) {
			for (auto j = green_min; j < green_max + 1; j++) {
				for (auto k = blue_min; k < blue_max + 1; k++) {
					const auto histoindex = GetColorIndex(i, j, k);
					const auto hval = histo_[histoindex];
					ntot += hval;
					r_sum += hval * (i + 0.5) * mult;
					g_sum += hval * (j + 0.5) * mult;
					b_sum += hval * (k + 0.5) * mult;
				}
			}
		}

		auto r_avg = 0;
		auto g_avg = 0;
		auto b_avg = 0;

		if (ntot > 0) {
			r_avg = static_cast<int32_t>(r_sum / ntot);
			g_avg = static_cast<int32_t>(g_sum / ntot);
			b_avg = static_cast<int32_t>(b_sum / ntot);
		}
		else {
			r_avg = static_cast<int32_t>(mult * (red_min + red_max + 1) / 2.0);
			g_avg = static_cast<int32_t>(mult * (green_min + green_max + 1) / 2.0);
			b_avg = static_cast<int32_t>(mult * (blue_min + blue_max + 1) / 2.0);
		}
		avg_ = std::make_tuple(r_avg, g_avg, b_avg);
	}

	static constexpr int32_t GetColorIndex(int32_t r, int32_t g, int32_t b) noexcept {
		return (r << (2 * SIGBITS)) + (g << SIGBITS) + b;
	}

	int32_t GetVolume() const noexcept {
		const auto sub_r = red_max - red_min;
		const auto sub_g = green_max - green_min;
		const auto sub_b = blue_max - blue_min;
		return (sub_r + 1) * (sub_g + 1) * (sub_b + 1);
	}

	int32_t GetCount() const noexcept {
		return count_;
	}

	std::tuple<int32_t, int32_t, int32_t> GetAvg() const noexcept {
		return avg_;
	}

	int32_t red_min;
	int32_t red_max;
	int32_t green_min;
	int32_t green_max;
	int32_t blue_min;
	int32_t blue_max;
private:
	int32_t count_;
	std::tuple<uint8_t, uint8_t, uint8_t> avg_;
	std::vector<int32_t> histo_;
};

struct ColorStatisQueueCompare final {
	bool operator()(const std::tuple<ColorStatis, Color>& left, const std::tuple<ColorStatis, Color>& right) const noexcept {
		auto l = std::get<0>(left);
		auto r = std::get<0>(right);
		return l.GetCount() * l.GetVolume()
			< r.GetCount() * r.GetVolume();
	}
};

struct ColorStatisMultiplyCompare final {
	bool operator()(const ColorStatis& left, const ColorStatis& right) const noexcept {
		return left.GetCount() * left.GetVolume()
			< right.GetCount() * right.GetVolume();
	}
};

struct ColorStatisCountCompare final {
	bool operator()(const ColorStatis& left, const ColorStatis& right) const noexcept {
		return left.GetCount() < right.GetCount();
	}
};

template <typename T, typename Compare>
class SortedQueue final {
public:
	SortedQueue()
		: is_sorted_(false) {
	}

	T Pop() {
		if (!is_sorted_) {
			Sort();
		}
		const auto& result = contents_.back();
		contents_.pop_back();
		return result;
	}

	void Push(const T& value) {
		contents_.emplace_back(value);
	}

	const std::vector<T>& GetContents() const {
		return contents_;
	}

	size_t GetSize() const {
		return contents_.size();
	}

private:
	void Sort() {
		std::sort(contents_.begin(), contents_.end(), compare_);
		is_sorted_ = true;
	}
	bool is_sorted_;
	std::vector<T> contents_;
	Compare compare_;
};

class ColorStatisQueue final {
public:
	ColorStatisQueue() = default;

	std::vector<QColor> GetPalette() const {
		std::vector<QColor> colors;
		colors.reserve(queue_.GetContents().size());
		for (auto& [_, avg_color] : queue_.GetContents()) {
			colors.emplace_back(
				std::get<0>(avg_color),
				std::get<1>(avg_color),
				std::get<2>(avg_color)
			);
		}
		return colors;
	}

	void Push(const ColorStatis& statis) {
		queue_.Push({ statis, statis.GetAvg() });
	}

	const std::vector<std::tuple<ColorStatis, Color>>& GetContents() const {
		return queue_.GetContents();
	}

	size_t GetSize() const {
		return queue_.GetSize();
	}
private:
	SortedQueue<std::tuple<ColorStatis, Color>, ColorStatisQueueCompare> queue_;
};

static ColorStatis FromPixels(const std::vector<Color>& pixels, std::vector<int32_t>& histo) {
	auto rmin = 1000000;
	auto rmax = 0;
	auto gmin = 1000000;
	auto gmax = 0;
	auto bmin = 1000000;
	auto bmax = 0;

	for (const Color& pixel : pixels) {
		const auto red = std::get<0>(pixel) >> ColorStatis::RSHIFT;
		const auto green = std::get<1>(pixel) >> ColorStatis::RSHIFT;
		const auto blue = std::get<2>(pixel) >> ColorStatis::RSHIFT;
		rmin = (std::min)(red, rmin);
		rmax = (std::max)(red, rmax);
		gmin = (std::min)(green, gmin);
		gmax = (std::max)(green, gmax);
		bmin = (std::min)(blue, bmin);
		bmax = (std::max)(blue, bmax);
	}
	return { rmin, rmax, gmin, gmax, bmin, bmax, histo };
}

static std::vector<int32_t> GetHisto(const std::vector<Color>& pixels) {
	std::vector<int32_t> histo(std::pow(2, 3 * ColorStatis::SIGBITS), 0);

	for (const Color& pixel : pixels) {
		const auto red = std::get<0>(pixel) >> ColorStatis::RSHIFT;
		const auto green = std::get<1>(pixel) >> ColorStatis::RSHIFT;
		const auto blue = std::get<2>(pixel) >> ColorStatis::RSHIFT;
		const auto index = ColorStatis::GetColorIndex(red, green, blue);
		histo[index] += 1;
	}
	return histo;
}

enum class CutColor {
	kRed,
	kGreen,
	kBlue,
};

#define SUM_LOOP(color1_min, color1_max, color2_min, color2_max, color3_min, color3_max) \
for (auto i = color1_min; i < color1_max + 1; ++i) { \
	auto sum = 0;\
	for (int j = color2_min; j < color2_max + 1; j++) {\
		for (int k = color3_min; k < color3_max + 1; k++) {\
			const auto index = ColorStatis::GetColorIndex(i, j, k);\
			sum += histo[index];\
		}\
	}\
	total += sum;\
	partial_sum[i] = total;\
}

static std::tuple<std::optional<ColorStatis>, std::optional<ColorStatis>> GetMedianCut(const std::vector<int32_t>& histo, const ColorStatis& statis) {
	auto rw = statis.red_max - statis.red_min + 1;
	auto gw = statis.green_max - statis.green_min + 1;
	auto bw = statis.blue_max - statis.blue_min + 1;
	auto maxw = std::max(rw, std::max(gw, bw));

	if (statis.GetCount() == 1) {
		return { {statis}, {} };
	}

	using xamp::base::HashMap;

	auto total = 0;

	HashMap<int32_t, int32_t> partial_sum;
	HashMap<int32_t, int32_t> lookahead_sum;
	CutColor do_cut_color = CutColor::kRed;

	if (maxw == rw) {
		do_cut_color = CutColor::kRed;
		SUM_LOOP(statis.red_min, statis.red_max, statis.green_min, statis.green_max, statis.blue_min, statis.blue_max)
	} else if (maxw == gw) {
		do_cut_color = CutColor::kGreen;
		SUM_LOOP(statis.green_min, statis.green_max, statis.red_min, statis.red_max, statis.blue_min, statis.blue_max)
	} else {
		do_cut_color = CutColor::kBlue;
		SUM_LOOP(
			statis.blue_min, statis.blue_max, statis.red_min, statis.red_max, statis.green_min, statis.green_max)
	}

	for (auto [i, d] : partial_sum) {
		lookahead_sum[i] = total - d;
	}

	auto dim1_val = 0;
	auto dim2_val = 0;

	if (do_cut_color == CutColor::kRed) {
		dim1_val = statis.red_min;
		dim2_val = statis.red_max;
	}
	else if (do_cut_color == CutColor::kGreen) {
		dim1_val = statis.green_min;
		dim2_val = statis.green_max;
	}
	else {
		dim1_val = statis.blue_min;
		dim2_val = statis.blue_max;
	}

	for (auto i = dim1_val; i < dim2_val + 1; ++i) {
		if (partial_sum[i] > total / 2) {
			auto s1 = statis;
			auto s2 = statis;

			auto left = i - dim1_val;
			auto right = dim2_val - i;
			auto d2 = 0;

			if (left <= right) {
				d2 = std::min(dim2_val - 1, static_cast<int>(i + right / 2.0));
			}
			else {
				d2 = std::max(dim1_val, static_cast<int>(i - 1 - left / 2.0));
			}

			while (!(partial_sum.count(d2) > 0 && partial_sum[d2] > 0)) {
				d2 += 1;
			}

			auto count2 = lookahead_sum[d2];
			while (count2 == 0 && partial_sum.count(d2 - 1) > 0 && partial_sum[d2 - 1] > 0) {
				d2 -= 1;
			}

			count2 = lookahead_sum[d2];

			switch (do_cut_color) {
			case CutColor::kRed:
				s1.red_max = d2;
				s2.red_min = s1.red_max + 1;
				break;
			case CutColor::kGreen:
				s1.green_max = d2;
				s2.green_min = s1.green_max + 1;
				break;
			case CutColor::kBlue:
				s1.blue_max = d2;
				s2.blue_min = s1.blue_max + 1;
				break;
			}
			return { {s1}, {s2} };
		}
	}

	return { {}, {} };
}

template <typename Compare>
static void MakeColorMedian(SortedQueue<ColorStatis, Compare>& CQ, double target, const std::vector<int32_t>& histo) {
	auto num_color = 1;
	auto num_iter = 0;

	while (num_iter < kMaxIteration) {
		const auto statis = CQ.Pop();

		if (statis.GetCount() == 0) {
			CQ.Push(statis);
			num_iter += 1;
			continue;
		}

		auto [s1, s2] = GetMedianCut(histo, statis);

		CQ.Push(s1.value());
		if (s2) {
			CQ.Push(s2.value());
			num_color += 1;
		}
		if (static_cast<double>(num_color) >= target || num_iter > kMaxIteration) {
			return;
		}
		num_iter += 1;
	}
}

static ColorStatisQueue Quantize(std::vector<Color>& pixels, int32_t max_color) {
	auto histo = GetHisto(pixels);
	auto statis = FromPixels(pixels, histo);

	SortedQueue<ColorStatis, ColorStatisCountCompare> count_queue;
	count_queue.Push(statis);

	MakeColorMedian(count_queue, kFractByPopulations * static_cast<double>(max_color), histo);

	SortedQueue<ColorStatis, ColorStatisMultiplyCompare> mul_queue;
	while (count_queue.GetSize() > 0) {
		mul_queue.Push(count_queue.Pop());
	}

	MakeColorMedian(mul_queue, static_cast<double>(max_color - mul_queue.GetSize()), histo);

	ColorStatisQueue result;
	while (mul_queue.GetSize() > 0) {
		result.Push(mul_queue.Pop());
	}
	return result;
}

std::vector<QColor> GetPalette(const QImage& image, int32_t color_count, int32_t quality) {
	const auto img = image.convertToFormat(QImage::Format_RGBA8888, Qt::AutoColor);

	/*std::vector<Color> pixels;
	pixels.reserve(img.byteCount() / 4);

	const auto* rgb = reinterpret_cast<const QRgb*>(img.constBits());
	const auto* const last = rgb + img.byteCount() / 4;

	for (; rgb != last; ++rgb) {
		if (qAlpha(*rgb) >= 125) {
			if (qRed(*rgb) < 250 || qGreen(*rgb) < 250 || qBlue(*rgb) < 250) {
				pixels.emplace_back(qRed(*rgb), qGreen(*rgb), qBlue(*rgb));
			}
		}
	}*/

	std::vector<Color> pixels;
	for (auto row = 0; row < img.height(); ++row) {
		for (auto col = 0; col < img.width(); ++col) {
			QColor rgba(img.pixel(row, col));
			if (rgba.alpha() >= 125) {
				if (rgba.red() < 250 || rgba.green() < 250 || rgba.blue() < 250) {
					pixels.emplace_back(rgba.red(), rgba.green(), rgba.blue());
				}
			}
		}
	}
	return Quantize(pixels, color_count).GetPalette();
}

std::vector<QColor> GetPalette(const QString& image_file_path, int32_t color_count, int32_t quality) {
	QImage image;
	if (!image.load(image_file_path)) {
		return {};
	}
	return GetPalette(image, color_count, quality);
}