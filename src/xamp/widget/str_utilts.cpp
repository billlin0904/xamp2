#include <QTime>
#include <widget/str_utilts.h>

QString colorToString(QColor color) {
    return QString(Q_UTF8("rgba(%1,%2,%3,%4)"))
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alpha());
}

QString backgroundColorToString(QColor color) {
    return Q_UTF8("background-color: ") + colorToString(color) + Q_UTF8(";");
}

QString bitRate2String(uint32_t bitRate) {
    if (bitRate > 10000) {
        return QString(Q_UTF8("%0 Mbps")).arg(QString::number(bitRate / 1000.0, 'f', 2));
    }
    return QString(Q_UTF8("%0 Kbps")).arg(bitRate);
}

QString samplerate2String(uint32_t samplerate) {
    auto precision = 1;
    auto is_mhz_samplerate = false;
    if (samplerate / 1000 > 1000) {
        is_mhz_samplerate = true;
    }
    else {
        precision = samplerate % 1000 == 0 ? 0 : 1;
    }

    return (is_mhz_samplerate ? QString::number(samplerate / 1000000.0, 'f', 2) + Q_UTF8(" MHz")
        : QString::number(samplerate / 1000.0, 'f', precision) + Q_UTF8("kHz"));
}

QString dsdSampleRate2String(uint32_t dsd_speed) {
    const auto sample_rate = (dsd_speed / 64) * 2.82;
    return QString::number(sample_rate, 'f', 2) + Q_UTF8("kHz");
}

QString msToString(const double stream_time) {
    const auto ms = static_cast<int32_t>(stream_time * 1000.0) % 1000;
    const auto secs = static_cast<int32_t>(stream_time);
    const auto h = secs / 3600;
    const auto m = (secs % 3600) / 60;
    const auto s = (secs % 3600) % 60;
    QTime t(h, m, s, ms);
    if (h > 0) {
        return t.toString(Q_UTF8("hh:mm:ss"));
    }
    return t.toString(Q_UTF8("mm:ss"));
}
