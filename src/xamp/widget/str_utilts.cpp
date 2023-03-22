#include <widget/appsettings.h>

#include <QDir>
#include <QLocale>
#include <QTime>
#include <QUuid>

#include <widget/str_utilts.h>

QString ColorToString(QColor color) {
    return QString(qTEXT("rgba(%1,%2,%3,%4)"))
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alpha());
}

QString BackgroundColorToString(QColor color) {
    return qTEXT("background-color: ") + ColorToString(color) + qTEXT(";");
}

QString FormatBitRate(uint32_t bitRate) {
    if (bitRate > 10000) {
        return QString::number(bitRate / 1000.0, 'f', 2) + qTEXT(" Mbps");
    }
    return QString::number(bitRate) + qTEXT(" kbps");
}

QString FormatSampleRate(const uint32_t sample_rate) {
    auto precision = 1;
    auto is_mhz_sample_rate = false;

    if (sample_rate / 1000 > 1000) {
        is_mhz_sample_rate = true;
    }
    if (is_mhz_sample_rate) {
        return QString::number(sample_rate / 1000000.0, 'f', 2) + qTEXT(" MHz");
    } else {
        return QString::number(sample_rate / 1000.0, 'f', precision) + qTEXT(" kHz");
    }
}

QString FormatDsdSampleRate(uint32_t dsd_speed) {
    const auto sample_rate = (dsd_speed / 64) * 2.82;
    return QString::number(sample_rate, 'f', 2) + qTEXT("kHz");
}

QString FormatDuration(const double stream_time, bool full_text) {
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

bool IsMoreThan1Hours(const double stream_time) {
    const auto ms = static_cast<int32_t>(stream_time * 1000.0) % 1000;
    const auto secs = static_cast<int32_t>(stream_time);
    const auto h = secs / 3600;
    return h > 0;
}

QString ToNativeSeparators(const QString& path) {
    return QDir::toNativeSeparators(path);
}

QByteArray GenerateUuid() {
    return QUuid::createUuid().toByteArray(QUuid::WithoutBraces);
}

QString FormatBytes(quint64 bytes) {
    return QString::fromStdString(String::FormatBytes(bytes));
}

QString FormatTime(quint64 time) {
    QDateTime date_time;
    date_time.setSecsSinceEpoch(time);
    return date_time.toString(qTEXT("yyyy-MM-dd"));
}

QString FormatDb(double value, int prec) {
    return qSTR("%1 dB").arg(FormatDouble(value, prec));
}

QString FormatDouble(double value, int prec) {
    return QString::number(value, 'f', 1);
}