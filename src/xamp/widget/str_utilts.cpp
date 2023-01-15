#include <QDir>
#include <QLocale>
#include <QTime>

#include <widget/appsettings.h>
#include <widget/str_utilts.h>

QString colorToString(QColor color) {
    return QString(qTEXT("rgba(%1,%2,%3,%4)"))
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alpha());
}

QString backgroundColorToString(QColor color) {
    return qTEXT("background-color: ") + colorToString(color) + qTEXT(";");
}

QString formatBitRate(uint32_t bitRate) {
    if (bitRate > 10000) {
        return QString::number(bitRate / 1000.0, 'f', 2).rightJustified(5) + qTEXT(" Mbps");
    }
    return QString::number(bitRate).rightJustified(5) + qTEXT(" kbps");
}

QString formatSampleRate(const uint32_t sample_rate) {
    auto precision = 1;
    auto is_mhz_sample_rate = false;

    if (sample_rate / 1000 > 1000) {
        is_mhz_sample_rate = true;
    }
    else {
        precision = sample_rate % 1000 == 0 ? 0 : 1;
    }

    if (is_mhz_sample_rate) {
        return QString::number(sample_rate / 1000000.0, 'f', 2).rightJustified(6) + qTEXT(" MHz");
    } else {
        return QString::number(sample_rate / 1000.0, 'f', precision).rightJustified(3) + qTEXT(" kHz");
    }
}

QString formatDsdSampleRate(uint32_t dsd_speed) {
    const auto sample_rate = (dsd_speed / 64) * 2.82;
    return QString::number(sample_rate, 'f', 2) + qTEXT("kHz");
}

QString formatDuration(const double stream_time, bool full_text) {
    const auto ms = static_cast<int32_t>(stream_time * 1000.0) % 1000;
    const auto secs = static_cast<int32_t>(stream_time);
    const auto h = secs / 3600;
    const auto m = (secs % 3600) / 60;
    const auto s = (secs % 3600) % 60;
    const QTime t(h, m, s, ms);
    if (h > 0 || full_text) {
        return QLocale().toString(t, qTEXT("HH:mm:ss"));
    }
    return QLocale().toString(t, qTEXT("mm:ss"));
}

bool isMoreThan1Hours(const double stream_time) {
    const auto ms = static_cast<int32_t>(stream_time * 1000.0) % 1000;
    const auto secs = static_cast<int32_t>(stream_time);
    const auto h = secs / 3600;
    return h > 0;
}

QString toNativeSeparators(const QString& path) {
    return QDir::toNativeSeparators(path);
}

QByteArray generateUUID() {
    return QUuid::createUuid().toByteArray(QUuid::WithoutBraces);
}

QString formatBytes(quint64 bytes) {
    return QString::fromStdString(String::FormatBytes(bytes));
}

QString formatTime(quint64 time) {
    return QDateTime::fromSecsSinceEpoch(time)
        .toString(qTEXT("yyyy-MM-dd HH:mm:ss"));
}
