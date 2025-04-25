#include <widget/util/str_util.h>
#include <widget/appsettings.h>

#include <QDir>
#include <QLocale>
#include <QTime>
#include <QUuid>

QString colorToString(QColor color) {
    return QString("rgba(%1,%2,%3,%4)"_str)
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alpha());
}

QString backgroundColorToString(QColor color) {
    return "background-color: "_str + colorToString(color) + ";"_str;
}

QString formatBitRate(uint32_t bit_rate) {
    if (bit_rate > 10000) {
        return QString::number(bit_rate / 1000.0, 'f', 2) + " Mbps"_str;
    }
    return QString::number(bit_rate) + " kbps"_str;
}

QString formatSampleRate(const uint32_t sample_rate) {
    auto precision = 1;
    auto is_mhz_sample_rate = false;

    if (sample_rate / 1000 > 1000) {
        is_mhz_sample_rate = true;
    }
    if (is_mhz_sample_rate) {
        return QString::number(sample_rate / 1000000.0, 'f', 2) + " MHz"_str;
    } else if (sample_rate > 1000.0) {
        return QString::number(sample_rate / 1000.0, 'f', precision) + " kHz"_str;
    } else {
        return QString::number(sample_rate) + " Hz"_str;
    }
}

QString formatDsdSampleRate(uint32_t dsd_speed) {
    const auto sample_rate = (dsd_speed / 64) * 2.82;
    return QString::number(sample_rate, 'f', 2) + "kHz"_str;
}

bool parseVersion(const QString& s, QVersionNumber& version) {
    const auto ver = s.split("."_str);
    if (ver.length() != 3) {
        return false;
    }

    auto major_part = 0;
    auto minor_part = 0;
    auto revision_part = 0;

    for (auto i = 0; i < ver.length(); ++i) {
        bool ok = false;
        switch (i) {
        case 0:
            major_part = ver[i].toInt(&ok);
            break;
        case 1:
            minor_part = ver[i].toInt(&ok);
            break;
        case 2:
            revision_part = ver[i].toInt(&ok);
            break;
        default:;
        }
        if (!ok) {
            return false;
        }
    }
    version = QVersionNumber(major_part, minor_part, revision_part);
    return true;
}

QString formatDurationAsMinutes(const double stream_time) {
    const auto secs = static_cast<int32_t>(stream_time);
    const auto ms = static_cast<int32_t>(stream_time * 1000.0) % 1000;

    // 時間超過 60 分鐘：以累計分鐘來顯示，不顯示小時部分
    const auto total_minutes = secs / 60;
    const auto s = secs % 60;
    // 補零格式化，確保分鐘和秒數各兩位數
    return QString("%1:%2"_str)
        .arg(total_minutes, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}

QString formatDuration(const double stream_time, bool full_text) {
    const auto ms = static_cast<int32_t>(stream_time * 1000.0) % 1000;
    const auto secs = static_cast<int32_t>(stream_time);
    const auto h = secs / 3600;
    const auto m = (secs % 3600) / 60;
    const auto s = (secs % 3600) % 60;
    const QTime t(h, m, s, ms);
    if (h > 0 || full_text) {
        return t.toString("HH:mm:ss"_str);
    }
    return t.toString("mm:ss"_str);
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
    return date_time.toString("yyyy-MM-dd"_str);
}

QString formatVersion(const QVersionNumber& version) {
    return version.toString();
}

QString formatDb(double value, int prec) {
    return qFormat("%1 dB").arg(formatDouble(value, prec));
}

QString formatDouble(double value, int prec) {
    return QString::number(value, 'f', prec);
}

int32_t countColon(const std::string& str) {
    int32_t count = 0;
    size_t pos = str.find(':');
    while (pos != std::string::npos) {
        count++;
        pos = str.find(':', pos + 1);
    }
    return count;
}

double parseDuration(const std::string & str) {
    auto hours = 0;
    auto minutes = 0;
    auto seconds = 0;

    
    if (countColon(str) == 1) {
        port_sscanf(str.c_str(), "%u:%u",
            &minutes,
            &seconds);
    } else {
        port_sscanf(str.c_str(), "%u:%u:%u",
            &hours,
            &minutes,
            &seconds);
    }

    const std::chrono::milliseconds duration = std::chrono::hours(hours)
        + std::chrono::minutes(minutes)
        + std::chrono::seconds(seconds);

    return duration.count() / 1000.0;
}