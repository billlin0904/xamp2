#include <widget/str_utilts.h>
#include <widget/appsettings.h>

#include <QDir>
#include <QLocale>
#include <QTime>
#include <QUuid>

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

QString formatBitRate(uint32_t bit_rate) {
    if (bit_rate > 10000) {
        return QString::number(bit_rate / 1000.0, 'f', 2) + qTEXT(" Mbps");
    }
    return QString::number(bit_rate) + qTEXT(" kbps");
}

QString formatSampleRate(const uint32_t sample_rate) {
    auto precision = 1;
    auto is_mhz_sample_rate = false;

    if (sample_rate / 1000 > 1000) {
        is_mhz_sample_rate = true;
    }
    if (is_mhz_sample_rate) {
        return QString::number(sample_rate / 1000000.0, 'f', 2) + qTEXT(" MHz");
    } else if (sample_rate > 1000.0) {
        return QString::number(sample_rate / 1000.0, 'f', precision) + qTEXT(" kHz");
    } else {
        return QString::number(sample_rate) + qTEXT(" Hz");
    }
}

QString formatDsdSampleRate(uint32_t dsd_speed) {
    const auto sample_rate = (dsd_speed / 64) * 2.82;
    return QString::number(sample_rate, 'f', 2) + qTEXT("kHz");
}

bool parseVersion(const QString& s, Version& version) {
    const auto ver = s.split(qTEXT("."));
    if (ver.length() != 3) {
        return false;
    }

    for (auto i = 0; i < ver.length(); ++i) {
        bool ok = false;
        switch (i) {
        case 0:
            version.major_part = ver[i].toInt(&ok);
            break;
        case 1:
            version.minor_part = ver[i].toInt(&ok);
            break;
        case 2:
            version.revision_part = ver[i].toInt(&ok);
            break;
        default:;
        }
        if (!ok) {
            return false;
        }
    }
    return true;
}

QString formatDuration(const double stream_time, bool full_text) {
    const auto ms = static_cast<int32_t>(stream_time * 1000.0) % 1000;
    const auto secs = static_cast<int32_t>(stream_time);
    const auto h = secs / 3600;
    const auto m = (secs % 3600) / 60;
    const auto s = (secs % 3600) % 60;
    const QTime t(h, m, s, ms);
    if (h > 0 || full_text) {
        return t.toString(qTEXT("HH:mm:ss"));
    }
    return t.toString(qTEXT("mm:ss"));
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

QByteArray generateUuid() {
    return QUuid::createUuid().toByteArray(QUuid::WithoutBraces);
}

QString formatBytes(quint64 bytes) {
    return QString::fromStdString(String::FormatBytes(bytes));
}

QString formatTime(quint64 time) {
    QDateTime date_time;
    date_time.setSecsSinceEpoch(time);
    return date_time.toString(qTEXT("yyyy-MM-dd"));
}

QString formatVersion(const Version& version) {
    return qSTR("%1.%2.%3").arg(version.major_part)
        .arg(version.minor_part)
        .arg(version.revision_part);
}

QString formatDb(double value, int prec) {
    return qSTR("%1 dB").arg(formatDouble(value, prec));
}

QString formatDouble(double value, int prec) {
    return QString::number(value, 'f', prec);
}