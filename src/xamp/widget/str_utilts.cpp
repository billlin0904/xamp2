#include <QDir>
#include <QTime>
#include <widget/str_utilts.h>

QString colorToString(QColor color) {
    return QString(Q_TEXT("rgba(%1,%2,%3,%4)"))
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alpha());
}

QString backgroundColorToString(QColor color) {
    return Q_TEXT("background-color: ") + colorToString(color) + Q_TEXT(";");
}

QString bitRate2String(uint32_t bitRate) {
    if (bitRate > 10000) {
        return QString::number(bitRate / 1000.0, 'f', 2).rightJustified(5) + Q_TEXT(" Mbps");
    }
    return QString::number(bitRate).rightJustified(5) + Q_TEXT(" Kbps");
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

    if (is_mhz_samplerate) {
        return QString::number(samplerate / 1000000.0, 'f', 2).rightJustified(6) + Q_TEXT(" MHz");
    } else {
        return QString::number(samplerate / 1000.0, 'f', precision).rightJustified(3) + Q_TEXT(" kHz");
    }
}

QString dsdSampleRate2String(uint32_t dsd_speed) {
    const auto sample_rate = (dsd_speed / 64) * 2.82;
    return QString::number(sample_rate, 'f', 2) + Q_TEXT("kHz");
}

QString msToString(const double stream_time, bool full_text) {
    const auto ms = static_cast<int32_t>(stream_time * 1000.0) % 1000;
    const auto secs = static_cast<int32_t>(stream_time);
    const auto h = secs / 3600;
    const auto m = (secs % 3600) / 60;
    const auto s = (secs % 3600) % 60;
    QTime t(h, m, s, ms);
    if (h > 0 || full_text) {
        return t.toString(Q_TEXT("hh:mm:ss"));
    }
    return t.toString(Q_TEXT("mm:ss"));
}

bool isMoreThan1Hours(const double stream_time) {
    const auto ms = static_cast<int32_t>(stream_time * 1000.0) % 1000;
    const auto secs = static_cast<int32_t>(stream_time);
    const auto h = secs / 3600;
    return h > 0;
}

QString fromQStringPath(const QString& path) {
    return QDir::toNativeSeparators(path);
}
