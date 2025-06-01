#include <widget/widget_shared.h>
#include <widget/util/colortable.h>
#include <base/base.h>
#include <base/math.h>

const std::array<QRgb, ColorTable::kLutSize> ColorTable::kSoxrLut = [] noexcept {
    std::array<QRgb, kLutSize> lut{};
    for (std::size_t i = 0; i < kLutSize; ++i) {
        const double ratio = static_cast<double>(i) / (kLutSize - 1);
        lut[i] = soxrColor(ratio);
    }
    return lut;
    }();

const std::array<QRgb, ColorTable::kLutSize> ColorTable::kDanBrutonLut = [] noexcept {
    std::array<QRgb, kLutSize> lut{};
    for (std::size_t i = 0; i < kLutSize; ++i) {
        const double ratio = static_cast<double>(i) / (kLutSize - 1);
        lut[i] = danBrutonColor(ratio);
    }
    return lut;
    }();

ColorTable::ColorTable() {
    color_lut_ptr_ = kSoxrLut.data();
}

void ColorTable::setSpectrogramColor(SpectrogramColor color) {
    color_ = color;
    color_lut_ptr_ = (color_ == SpectrogramColor::SPECTROGRAM_COLOR_DEFAULT)
        ? kDanBrutonLut.data()
        : kSoxrLut.data();
}

QRgb ColorTable::operator[](double dB_val) const noexcept {
    dB_val = std::clamp(dB_val, kMinDb, kMaxDb);
    const double ratio = (dB_val - kMinDb) / kDbRange;    
    /*if (color_ == SpectrogramColor::SPECTROGRAM_COLOR_DEFAULT) {
        return danBrutonColor(ratio);
    }
    return soxrColor(ratio);*/
    const size_t idx = static_cast<size_t>(ratio * (kLutSize - 1));
    return color_lut_ptr_[idx];
}

QRgb ColorTable::danBrutonColor(double level) noexcept {
    level *= 0.6625;
    double r = 0.0, g = 0.0, b = 0.0;
    if (level >= 0 && level < 0.15) {
        r = (0.15 - level) / (0.15 + 0.075);
        g = 0.0;
        b = 1.0;
    }
    else if (level >= 0.15 && level < 0.275) {
        r = 0.0;
        g = (level - 0.15) / (0.275 - 0.15);
        b = 1.0;
    }
    else if (level >= 0.275 && level < 0.325) {
        r = 0.0;
        g = 1.0;
        b = (0.325 - level) / (0.325 - 0.275);
    }
    else if (level >= 0.325 && level < 0.5) {
        r = (level - 0.325) / (0.5 - 0.325);
        g = 1.0;
        b = 0.0;
    }
    else if (level >= 0.5 && level < 0.6625) {
        r = 1.0;
        g = (0.6625 - level) / (0.6625 - 0.5f);
        b = 0.0;
    }

    // Intensity correction.
    double cf = 1.0;
    if (level >= 0.0 && level < 0.1) {
        cf = level / 0.1;
    }
    cf *= 255.0;

    // Pack RGB values into a 32-bit uint.
    auto rr = static_cast<uint32_t>(r * cf + 0.5);
    auto gg = static_cast<uint32_t>(g * cf + 0.5);
    auto bb = static_cast<uint32_t>(b * cf + 0.5);

    return qRgb(rr, gg, bb);
}

QRgb ColorTable::soxrColor(double level) noexcept {
    double r = 0.0;
    if (level >= 0.13 && level < 0.73) {
        r = sin((level - 0.13) / 0.60 * XAMP_PI / 2.0);
    }
    else if (level >= 0.73) {
        r = 1.0;
    }

    double g = 0.0;
    if (level >= 0.6 && level < 0.91) {
        g = sin((level - 0.6) / 0.31 * XAMP_PI / 2.0);
    }
    else if (level >= 0.91) {
        g = 1.0;
    }

    double b = 0.0;
    if (level < 0.60) {
        b = 0.5 * sin(level / 0.6 * XAMP_PI);
    }
    else if (level >= 0.78) {
        b = (level - 0.78) / 0.22;
    }

    // clamp b
    if (b < 0.) b = 0.;
    if (b > 1.) b = 1.;

    auto rr = static_cast<uint32_t>(r * 255.0 + 0.5);
    auto gg = static_cast<uint32_t>(g * 255.0 + 0.5);
    auto bb = static_cast<uint32_t>(b * 255.0 + 0.5);

    return qRgb(rr, gg, bb);}
