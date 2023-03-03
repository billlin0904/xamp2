#include <ostream>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <algorithm>
#include <optional>
#include <QImage>

#include <widget/colorthief.h>

constexpr int kSigbits = 5;
constexpr int kRshift = 8 - kSigbits;
constexpr double kFractByPopulations = 0.75;
constexpr int kMaxIteration = 1000;

int get_color_index(int r, int g, int b) {
    return (r << (2 * kSigbits)) + (g << kSigbits) + b;
}

class VBox {
public:
    VBox(int r1, int r2, int g1, int g2, int b1, int b2, std::vector<int>* histo)
	    : avg_initialized_(false)
	    , count_initialized_(false)
	    , count_cache_(0)
		, r1_(r1)
	    , r2_(r2)
	    , g1_(g1)
	    , g2_(g2)
	    , b1_(b1)
	    , b2_(b2)
	    , histo_(histo) {
    }

    int volume() const {
        auto sub_r = r2_ - r1_;
        auto sub_g = g2_ - g1_;
        auto sub_b = b2_ - b1_;
        return (sub_r + 1) * (sub_g + 1) * (sub_b + 1);
    }

    VBox copy() const {
	    return VBox(r1_, r2_, g1_, g2_, b1_, b2_, histo_);
    }

    QuantizedColor avg() const {
    	if (!avg_initialized_) {
    		init_avg();
    	}
    	return avg_cache_;
    }

    bool contains(const QuantizedColor& pixel) const {
        auto rval = std::get<0>(pixel) >> kRshift;
        auto gval = std::get<1>(pixel) >> kRshift;
        auto bval = std::get<2>(pixel) >> kRshift;
        return rval >= r1_ && rval <= r2_ &&
            gval >= g1_ && gval <= g2_ &&
            bval >= b1_ && bval <= b2_;
    }

    int count() const {
	    if (!count_initialized_) {
		    init_count();
	    }
    	return count_cache_;
    }

    void init_avg() const {
        auto ntot = 0;
        auto mult = 1 << (8 - kSigbits);
        double r_sum = 0;
        double g_sum = 0;
        double b_sum = 0;

        for (auto i = r1_; i < r2_ + 1; i++) {
            for (auto j = g1_; j < g2_ + 1; j++) {
                for (auto k = b1_; k < b2_ + 1; k++) {
	                const auto histoindex = get_color_index(i, j, k);
                    const auto hval = (*histo_)[histoindex];
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
            r_avg = static_cast<int>(r_sum / ntot);
            g_avg = static_cast<int>(g_sum / ntot);
            b_avg = static_cast<int>(b_sum / ntot);
        }
        else {
            r_avg = static_cast<int>(mult * (r1_ + r2_ + 1) / 2.0);
            g_avg = static_cast<int>(mult * (g1_ + g2_ + 1) / 2.0);
            b_avg = static_cast<int>(mult * (b1_ + b2_ + 1) / 2.0);
        }

        avg_cache_ = std::make_tuple(r_avg, g_avg, b_avg);
        avg_initialized_ = true;
    }

    void init_count() const {
        auto npix = 0;
        for (auto i = r1_; i < r2_ + 1; i++) {
            for (auto j = g1_; j < g2_ + 1; j++) {
                for (auto k = b1_; k < b2_ + 1; k++) {
	                const auto index = get_color_index(i, j, k);
                    npix += (*histo_)[index];
                }
            }
        }
        count_cache_ = npix;
        count_initialized_ = true;
    }

    mutable bool avg_initialized_;
    mutable bool count_initialized_;
    mutable int count_cache_;
    int r1_;
    int r2_;
    int g1_;
    int g2_;
    int b1_;
    int b2_;
    std::vector<int>* histo_;
    mutable QuantizedColor avg_cache_;
};

std::ostream& operator<<(std::ostream& os, VBox& box) {
    os << box.r1_ << "-" << box.r2_ << " " << box.g1_ << "-" << box.g2_ << " " << box.b1_ << "-" << box.b2_ << " Count: " << box.count() << " Volume: " << box.volume() << " Count * volume: " << uint64_t(box.count()) * uint64_t(box.volume());
    return os;
}

struct BoxCountCompare {
    bool operator()(const VBox& a, const VBox& b) const {
        return a.count() < b.count();
    }
};

struct BoxCountVolumeCompare {
    bool operator()(const VBox& a, const VBox& b) const {
        return static_cast<uint64_t>(a.count()) * static_cast<uint64_t>(a.volume()) < static_cast<uint64_t>(b.count()) * static_cast<uint64_t>(b.volume());
    }
};

struct CMapCompare {
    bool operator()(const std::tuple<VBox, QuantizedColor>& a, const std::tuple<VBox, QuantizedColor>& b) const {
    	const auto &box1 = std::get<0>(a);
        const auto &box2 = std::get<0>(b);
    	return static_cast<uint64_t>(box1.count()) * static_cast<uint64_t>(box1.volume()) < static_cast<uint64_t>(box2.count()) * static_cast<uint64_t>(box2.volume());
    }
};

template <typename T, typename TCompare>
class PQueue {
public:
	PQueue()
		: sorted_(false) {
    }

    void sort() {
        std::sort(contents_.begin(), contents_.end(), compare_);
        sorted_ = true;
    }

    void reserve(size_t size) {
        contents_.reserve(size);
	}

    void push(const T& o) {
        contents_.push_back(o);
        sorted_ = false;
    }

    T pop() {
        if (!sorted_) {
            sort();
        }

        T result = contents_.back();
        contents_.pop_back();
        return result;
    }

    size_t size() const noexcept {
		return contents_.size();
	}

    const std::vector<T> & get_contents() const {
		return contents_;
	}

private:
    bool sorted_;
    TCompare compare_;
    std::vector<T> contents_;
};

template <typename TCompare>
class CMap {
public:
    CMap() = default;

    std::vector<QuantizedColor> pallete() const {
        std::vector<QuantizedColor> colors;
        colors.reserve(vboxes_.size());
        for (const auto& [vbox, avg_color] : vboxes_.get_contents()) {
            colors.push_back(avg_color);
        }
        return colors;
    }

    void reserve(size_t size) {
        vboxes_.reserve(size);
    }

    void push(VBox&& box) {
        vboxes_.push({ box, box.avg() });
    }

    size_t size() const noexcept {
		return vboxes_.size();
	}

private:
    PQueue<std::tuple<VBox, QuantizedColor>, TCompare> vboxes_;
};

std::vector<int> get_histo(const std::vector<QuantizedColor>& pixels) {
    std::vector<int> histo(std::pow(2, 3 * kSigbits), 0);

    for (const QuantizedColor& pixel : pixels) {
        int rval = std::get<0>(pixel) >> kRshift;
        int gval = std::get<1>(pixel) >> kRshift;
        int bval = std::get<2>(pixel) >> kRshift;
        int index = get_color_index(rval, gval, bval);
        histo[index] += 1;
    }
    return histo;
}

VBox vbox_from_pixels(const std::vector<QuantizedColor>& pixels, std::vector<int>& histo) {
    auto rmin = 1000000;
    auto rmax = 0;
    auto gmin = 1000000;
    auto gmax = 0;
    auto bmin = 1000000;
    auto bmax = 0;

    for (const QuantizedColor& pixel : pixels) {
        auto rval = std::get<0>(pixel) >> kRshift;
        auto gval = std::get<1>(pixel) >> kRshift;
        auto bval = std::get<2>(pixel) >> kRshift;
        rmin = std::min(rval, rmin);
        rmax = std::max(rval, rmax);
        gmin = std::min(gval, gmin);
        gmax = std::max(gval, gmax);
        bmin = std::min(bval, bmin);
        bmax = std::max(bval, bmax);
    }

    return VBox(rmin, rmax, gmin, gmax, bmin, bmax, &histo);
}

std::tuple<std::optional<VBox>, std::optional<VBox>> median_cut_apply(std::vector<int>& histo, VBox vbox) {
    auto rw = vbox.r2_ - vbox.r1_ + 1;
    auto gw = vbox.g2_ - vbox.g1_ + 1;
    auto bw = vbox.b2_ - vbox.b1_ + 1;
    auto maxw = std::max(rw, std::max(gw, bw));

    if (vbox.count() == 1)
        return { {vbox.copy()}, {} };

    auto total = 0;
    auto sum = 0;
    std::map<int, int> partialsum;
    std::map<int, int> lookaheadsum;
    char do_cut_color = '0';

    if (maxw == rw) {
        do_cut_color = 'r';
        for (auto i = vbox.r1_; i < vbox.r2_ + 1; ++i) {
            sum = 0;
            for (auto j = vbox.g1_; j < vbox.g2_ + 1; j++) {
                for (auto k = vbox.b1_; k < vbox.b2_ + 1; k++) {
                    auto index = get_color_index(i, j, k);
                    sum += histo[index];
                }
            }
            total += sum;
            partialsum[i] = total;
        }
    }
    else if (maxw == gw) {
        do_cut_color = 'g';
        for (auto i = vbox.g1_; i < vbox.g2_ + 1; ++i) {
            sum = 0;
            for (auto j = vbox.r1_; j < vbox.r2_ + 1; j++) {
                for (auto k = vbox.b1_; k < vbox.b2_ + 1; k++) {
                    auto index = get_color_index(j, i, k);
                    sum += histo[index];
                }
            }
            total += sum;
            partialsum[i] = total;
        }
    }
    else {
        do_cut_color = 'b';
        for (auto i = vbox.b1_; i < vbox.b2_ + 1; ++i) {
            sum = 0;
            for (auto j = vbox.r1_; j < vbox.r2_ + 1; j++) {
                for (auto k = vbox.g1_; k < vbox.g2_ + 1; k++) {
                    auto index = get_color_index(j, k, i);
                    sum += histo[index];
                }
            }
            total += sum;
            partialsum[i] = total;
        }
    }

    for (auto [i, d] : partialsum) {
        lookaheadsum[i] = total - d;
    }

    auto dim1_val = 0;
    auto dim2_val = 0;
    if (do_cut_color == 'r') {
        dim1_val = vbox.r1_;
        dim2_val = vbox.r2_;
    }
    else if (do_cut_color == 'g') {
        dim1_val = vbox.g1_;
        dim2_val = vbox.g2_;
    }
    else {
        dim1_val = vbox.b1_;
        dim2_val = vbox.b2_;
    }

    for (auto i = dim1_val; i < dim2_val + 1; ++i) {
        if (partialsum[i] > total / 2) {
            auto vbox1 = vbox.copy();
            auto vbox2 = vbox.copy();
            auto left = i - dim1_val;
            auto right = dim2_val - i;
            auto d2 = 0;
            if (left <= right) {
                d2 = std::min(dim2_val - 1, static_cast<int>(i + right / 2.0));
            }
            else {
                d2 = std::max(dim1_val, static_cast<int>(i - 1 - left / 2.0));
            }

            while (!(partialsum.count(d2) > 0 && partialsum[d2] > 0)) {
                d2 += 1;
            }

            int count2 = lookaheadsum[d2];
            while (count2 == 0 && partialsum.count(d2 - 1) > 0 && partialsum[d2 - 1] > 0) {
                d2 -= 1;
            }

            count2 = lookaheadsum[d2];

            if (do_cut_color == 'r') {
                vbox1.r2_ = d2;
                vbox2.r1_ = vbox1.r2_ + 1;
            }
            else if (do_cut_color == 'g') {
                vbox1.g2_ = d2;
                vbox2.g1_ = vbox1.g2_ + 1;
            }
            else {
                vbox1.b2_ = d2;
                vbox2.b1_ = vbox1.b2_ + 1;
            }

            return { vbox1, vbox2 };
        }
    }
    return { {}, {} };
}

template <typename TCompare>
void iter(PQueue<VBox, TCompare>& lh, double target, std::vector<int>& histo) {
    auto n_color = 1;
    auto n_iter = 0;
    while (n_iter < kMaxIteration) {
        VBox vbox = lh.pop();
        if (vbox.count() == 0) {
            lh.push(vbox);
            n_iter += 1;
            continue;
        }

        auto [vbox1, vbox2] = median_cut_apply(histo, vbox);

        if (!vbox1) {
            throw std::runtime_error("vbox1 not defined; shouldnt happen!");
        }

        lh.push(vbox1.value());
        if (vbox2) {
            lh.push(vbox2.value());
            n_color += 1;
        }
        if (static_cast<double>(n_color) >= target || n_iter > kMaxIteration) {
            return;
        }
        n_iter += 1;
    }
}

CMap<CMapCompare> quantize(const std::vector<QuantizedColor>& pixels, int max_color) {
    auto histo = get_histo(pixels);
    const auto vbox = vbox_from_pixels(pixels, histo);

    PQueue<VBox, BoxCountCompare> pq;
    pq.push(vbox);

    iter(pq, kFractByPopulations * static_cast<double>(max_color), histo);

    PQueue<VBox, BoxCountVolumeCompare> pq2;
    pq2.reserve(pq.size());
    while (pq.size() > 0) {
        pq2.push(pq.pop());
    }

    iter(pq2, max_color - pq2.size(), histo);

    CMap<CMapCompare> cmap;
    cmap.reserve(pq2.size());
    while (pq2.size() > 0) {
        cmap.push(pq2.pop());
    }
    return cmap;
}

void ColorThief::LoadImage(const QImage& image, int32_t color_count, int32_t quality, bool ignore_white) {
    auto cmap = quantize(GetPixels(image, quality, ignore_white), color_count);
    palette_.clear();
    palette_.reserve(cmap.size());
    for (auto avg_color : cmap.pallete()) {
        palette_.push_back(QColor(
            std::get<0>(avg_color),
            std::get<1>(avg_color),
            std::get<2>(avg_color))
        );
    }
}

const QList<QColor>& ColorThief::GetPalette() const {
    return palette_;
}

QColor ColorThief::GetDominantColor() const {
    return palette_[0];
}

std::vector<QuantizedColor> ColorThief::GetPixels(const QImage& image, int32_t quality, bool ignore_white) {
    const auto img = image.convertToFormat(QImage::Format_RGBA8888, Qt::AutoColor);

    std::vector<QuantizedColor> pixels;
    pixels.reserve(img.sizeInBytes() / 4);

    for (auto x = 0; x < img.width(); ++x) {
        for (auto y = 0; y < img.height(); ++y) {
            QColor rgba(img.pixel(x, y));
            if (rgba.alpha() >= 125) {
                if (!(ignore_white && rgba.red() > 250 && rgba.green() > 250 && rgba.blue() > 250)) {
                    pixels.emplace_back(rgba.red(), rgba.green(), rgba.blue());
                }
            }
        }
    }
    return pixels;
}