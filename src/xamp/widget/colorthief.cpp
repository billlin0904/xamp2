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
	    : avg_initialized(false)
	    , count_initialized(false)
	    , count_cache(0)
		, r1(r1)
	    , r2(r2)
	    , g1(g1)
	    , g2(g2)
	    , b1(b1)
	    , b2(b2)
	    , histo(histo) {
    }

    int volume() const {
        int sub_r = r2 - r1;
        int sub_g = g2 - g1;
        int sub_b = b2 - b1;
        return (sub_r + 1) * (sub_g + 1) * (sub_b + 1);
    }

    VBox copy() {
	    return VBox(r1, r2, g1, g2, b1, b2, histo);
    }

    color_t avg() {
    	if (!avg_initialized) {
    		init_avg();
    	}
    	return avg_cache;
    }

    bool contains(const color_t& pixel) {
        int rval = std::get<0>(pixel) >> kRshift;
        int gval = std::get<1>(pixel) >> kRshift;
        int bval = std::get<2>(pixel) >> kRshift;
        return rval >= r1 && rval <= r2 &&
            gval >= g1 && gval <= g2 &&
            bval >= b1 && bval <= b2;
    }

    int count() const {
	    if (!count_initialized) {
		    init_count();
	    }
    	return count_cache;
    }

    void init_avg() {
        int ntot = 0;
        int mult = 1 << (8 - kSigbits);
        double r_sum = 0;
        double g_sum = 0;
        double b_sum = 0;

        for (int i = r1; i < r2 + 1; i++) {
            for (int j = g1; j < g2 + 1; j++) {
                for (int k = b1; k < b2 + 1; k++) {
                    int histoindex = get_color_index(i, j, k);
                    int hval = (*histo)[histoindex];
                    ntot += hval;
                    r_sum += hval * (i + 0.5) * mult;
                    g_sum += hval * (j + 0.5) * mult;
                    b_sum += hval * (k + 0.5) * mult;
                }
            }
        }

        int r_avg;
        int g_avg;
        int b_avg;

        if (ntot > 0) {
            r_avg = static_cast<int>(r_sum / ntot);
            g_avg = static_cast<int>(g_sum / ntot);
            b_avg = static_cast<int>(b_sum / ntot);
        }
        else {
            r_avg = static_cast<int>(mult * (r1 + r2 + 1) / 2.0);
            g_avg = static_cast<int>(mult * (g1 + g2 + 1) / 2.0);
            b_avg = static_cast<int>(mult * (b1 + b2 + 1) / 2.0);
        }

        avg_cache = { r_avg, g_avg, b_avg };
        avg_initialized = true;
    }

    void init_count() const {
        int npix = 0;
        for (int i = r1; i < r2 + 1; i++) {
            for (int j = g1; j < g2 + 1; j++) {
                for (int k = b1; k < b2 + 1; k++) {
                    int index = get_color_index(i, j, k);
                    npix += (*histo)[index];
                }
            }
        }
        count_cache = npix;
        count_initialized = true;
    }

    bool avg_initialized;
    mutable bool count_initialized;
    mutable int count_cache;
    int r1;
    int r2;
    int g1;
    int g2;
    int b1;
    int b2;
    std::vector<int>* histo;
    color_t avg_cache;
};

std::ostream& operator<<(std::ostream& os, VBox& box) {
    os << box.r1 << "-" << box.r2 << " " << box.g1 << "-" << box.g2 << " " << box.b1 << "-" << box.b2 << " Count: " << box.count() << " Volume: " << box.volume() << " Count * volume: " << uint64_t(box.count()) * uint64_t(box.volume());
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
    bool operator()(const std::tuple<VBox, color_t>& a, const std::tuple<VBox, color_t>& b) const {
    	const auto &box1 = std::get<0>(a);
        const auto &box2 = std::get<0>(b);
    	return static_cast<uint64_t>(box1.count()) * static_cast<uint64_t>(box1.volume()) < static_cast<uint64_t>(box2.count()) * static_cast<uint64_t>(box2.volume());
    }
};

template <typename T, typename TCompare>
class PQueue {
public:
	PQueue()
		: contents_({})
		, sort_key_()
		, sorted_(false) {
    }

    void sort() {
        std::sort(contents_.begin(), contents_.end(), sort_key_);
        sorted_ = true;
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

    int size() {
		return contents_.size();
	}

    std::vector<T> get_contents() {
		return contents_;
	}

private:
    bool sorted_;
    TCompare sort_key_;
    std::vector<T> contents_;
};

template <typename TCompare>
class CMap {
public:
	CMap()
		: vboxes_() {
    }

    std::vector<color_t> pallete() {
        std::vector<color_t> colors;
        for (auto& [vbox, avg_color] : vboxes_.get_contents()) {
            colors.push_back(avg_color);
        }
        return colors;
    }

    void push(VBox&& box) {
        vboxes_.push({ box, box.avg() });
    }

    int size() {
		return vboxes_.size();
	}

private:
    PQueue<std::tuple<VBox, color_t>, TCompare> vboxes_;
};

std::vector<int> get_histo(const std::vector<color_t>& pixels) {
    std::vector<int> histo(std::pow(2, 3 * kSigbits), 0);

    for (const color_t& pixel : pixels) {
        int rval = std::get<0>(pixel) >> kRshift;
        int gval = std::get<1>(pixel) >> kRshift;
        int bval = std::get<2>(pixel) >> kRshift;
        int index = get_color_index(rval, gval, bval);
        histo[index] += 1;
    }
    return histo;
}

VBox vbox_from_pixels(const std::vector<color_t>& pixels, std::vector<int>& histo) {
    int rmin = 1000000;
    int rmax = 0;
    int gmin = 1000000;
    int gmax = 0;
    int bmin = 1000000;
    int bmax = 0;

    for (const color_t& pixel : pixels) {
        int rval = std::get<0>(pixel) >> kRshift;
        int gval = std::get<1>(pixel) >> kRshift;
        int bval = std::get<2>(pixel) >> kRshift;
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
    int rw = vbox.r2 - vbox.r1 + 1;
    int gw = vbox.g2 - vbox.g1 + 1;
    int bw = vbox.b2 - vbox.b1 + 1;
    int maxw = std::max(rw, std::max(gw, bw));

    if (vbox.count() == 1)
        return { {vbox.copy()}, {} };

    int total = 0;
    int sum = 0;
    std::unordered_map<int, int> partialsum;
    std::unordered_map<int, int> lookaheadsum;
    char do_cut_color = '0';

    if (maxw == rw) {
        do_cut_color = 'r';
        for (int i = vbox.r1; i < vbox.r2 + 1; ++i) {
            sum = 0;
            for (int j = vbox.g1; j < vbox.g2 + 1; j++) {
                for (int k = vbox.b1; k < vbox.b2 + 1; k++) {
                    int index = get_color_index(i, j, k);
                    sum += histo[index];
                }
            }
            total += sum;
            partialsum[i] = total;
        }
    }
    else if (maxw == gw) {
        do_cut_color = 'g';
        for (int i = vbox.g1; i < vbox.g2 + 1; ++i) {
            sum = 0;
            for (int j = vbox.r1; j < vbox.r2 + 1; j++) {
                for (int k = vbox.b1; k < vbox.b2 + 1; k++) {
                    int index = get_color_index(j, i, k);
                    sum += histo[index];
                }
            }
            total += sum;
            partialsum[i] = total;
        }
    }
    else {
        do_cut_color = 'b';
        for (int i = vbox.b1; i < vbox.b2 + 1; ++i) {
            sum = 0;
            for (int j = vbox.r1; j < vbox.r2 + 1; j++) {
                for (int k = vbox.g1; k < vbox.g2 + 1; k++) {
                    int index = get_color_index(j, k, i);
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

    int dim1_val;
    int dim2_val;
    if (do_cut_color == 'r') {
        dim1_val = vbox.r1;
        dim2_val = vbox.r2;
    }
    else if (do_cut_color == 'g') {
        dim1_val = vbox.g1;
        dim2_val = vbox.g2;
    }
    else {
        dim1_val = vbox.b1;
        dim2_val = vbox.b2;
    }

    for (int i = dim1_val; i < dim2_val + 1; ++i) {
        if (partialsum[i] > total / 2) {
            VBox vbox1 = vbox.copy();
            VBox vbox2 = vbox.copy();
            int left = i - dim1_val;
            int right = dim2_val - i;
            int d2;
            if (left <= right) {
                d2 = std::min(dim2_val - 1, int(i + right / 2.0));
            }
            else {
                d2 = std::max(dim1_val, int(i - 1 - left / 2.0));
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
                vbox1.r2 = d2;
                vbox2.r1 = vbox1.r2 + 1;
            }
            else if (do_cut_color == 'g') {
                vbox1.g2 = d2;
                vbox2.g1 = vbox1.g2 + 1;
            }
            else {
                vbox1.b2 = d2;
                vbox2.b1 = vbox1.b2 + 1;
            }

            return { vbox1, vbox2 };
        }
    }
    return { {}, {} };
}

template <typename TCompare>
void iter(PQueue<VBox, TCompare>& lh, double target, std::vector<int>& histo) {
    int n_color = 1;
    int n_iter = 0;
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

CMap<CMapCompare> quantize(const std::vector<color_t>& pixels, int max_color) {
    std::vector<int> histo = get_histo(pixels);
    const VBox vbox = vbox_from_pixels(pixels, histo);

    PQueue<VBox, BoxCountCompare> pq;
    pq.push(vbox);

    iter(pq, kFractByPopulations * static_cast<double>(max_color), histo);

    PQueue<VBox, BoxCountVolumeCompare> pq2;
    while (pq.size() > 0) {
        pq2.push(pq.pop());
    }

    iter(pq2, max_color - pq2.size(), histo);

    CMap<CMapCompare> cmap;
    while (pq2.size() > 0) {
        cmap.push(pq2.pop());
    }
    return cmap;
}

QList<QColor> ColorThief::GetPalette(const QImage& image, int32_t color_count, int32_t quality, bool ignore_white) {
    auto cmap = quantize(GetPixels(image, quality, ignore_white), color_count);
    QList<QColor> colors;
    colors.reserve(cmap.size());
    for (auto avg_color : cmap.pallete()) {
        colors.push_back(QColor(
            std::get<0>(avg_color),
            std::get<1>(avg_color),
            std::get<2>(avg_color))
        );
    }
    return colors;
}

std::vector<color_t> ColorThief::GetPixels(const QImage& image, int32_t quality, bool ignore_white) {
    const auto img = image.convertToFormat(QImage::Format_RGBA8888, Qt::AutoColor);

    std::vector<color_t> pixels;
    pixels.reserve(img.sizeInBytes() / 4);

    for (auto x = 0; x < img.width(); ++x) {
        for (auto y = 0; y < img.height(); ++y) {
            QColor rgba(img.pixel(x, y));
            if (rgba.alpha() >= 125) {
                if (rgba.red() < 250 || rgba.green() < 250 || rgba.blue() < 250) {
                    pixels.emplace_back(rgba.red(), rgba.green(), rgba.blue());
                }
            }
        }
    }
    return pixels;
}