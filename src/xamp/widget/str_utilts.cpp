#include <widget/str_utilts.h>

QString samplerate2String(uint32_t samplerate) {
    auto precision = 1;
    auto is_mhz_samplerate = false;
    if (samplerate / 1000 > 1000) {
        is_mhz_samplerate = true;
    }
    else {
        precision = samplerate % 1000 == 0 ? 0 : 1;
    }

    return (is_mhz_samplerate ? QString::number(samplerate / 1000000.0, 'f', 2) + Q_UTF8("MHz")
        : QString::number(samplerate / 1000.0, 'f', precision) + Q_UTF8("kHz"));
}