#include <widget/util/str_util.h>
#include <widget/appsettings.h>

#include <QDir>
#include <QLocale>
#include <QRegularExpression>
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

namespace {
    // ---- strip_non_alnum: 將非單字字元換成空白並 trim ----
    QString stripNonAlnum(const QString& s) {
        // \W 與 Python 類似：非「字母/數字/底線」
        static const QRegularExpression re(QStringLiteral("[\\W]+"));
        QString out = s;
        out.replace(re, QStringLiteral(" "));
        return out.trimmed();
    }

    // ---- normalize: 若去掉非字元後變空，就回傳原字串（忠實於 Python 版本）----
    QString normalize(const QString& orig) {
        const QString lowered = orig.toLower();
        const QString stripped = stripNonAlnum(lowered);
        return stripped.isEmpty() ? orig : stripped;
    }

    // ---- Levenshtein 距離，O(min(n,m)) 空間 ----
    int levenshteinDistance(const QString& aIn, const QString& bIn) {
        // 與 Python 版本同樣以 code unit（UTF-16）為單位
        const int n = aIn.size();
        const int m = bIn.size();
        if (n == 0) return m;
        if (m == 0) return n;

        // 確保 n <= m 以節省空間
        const QString* pa = &aIn;
        const QString* pb = &bIn;
        int nn = n, mm = m;
        if (nn > mm) { std::swap(pa, pb); std::swap(nn, mm); }

        QVector<int> current(nn + 1), previous(nn + 1);
        for (int i = 0; i <= nn; ++i) previous[i] = i;

        for (int j = 1; j <= mm; ++j) {
            current[0] = j;
            const QChar bj = (*pb)[j - 1];
            for (int i = 1; i <= nn; ++i) {
                const int add = previous[i] + 1;      // 刪除 pa[i-1]
                const int del = current[i - 1] + 1;   // 插入 pb[j-1]
                const int sub = previous[i - 1] + ((*pa)[i - 1] == bj ? 0 : 1);
                current[i] = std::min({ add, del, sub });
            }
            previous.swap(current);
        }
        return previous[nn];
    }

    // ---- astrcmp: 1 - dist/max(lenA,lenB) ----
    double astrcmp(const QString& a, const QString& b) {
        const int n = a.size();
        const int m = b.size();
        if (n == 0 && m == 0)
            return 0.0;  // 與 Python 分支對齊（其實這路徑不太會用到）
        if (n == 0 || m == 0)
            return 0.0;
        const int d = levenshteinDistance(a, b);
        const int L = std::max(n, m);
        return 1.0 - static_cast<double>(d) / static_cast<double>(L);
    }

    // ---- similarity: 對單字（先 normalize，再 astrcmp），與 Python 邏輯一致 ----
    double similarity(const QString& a1, const QString& b1) {
        const QString a2 = normalize(a1);
        const QString b2 = a2.isEmpty() ? QString() : normalize(b1);
        return astrcmp(a2, b2);
    }
}

// ---- similarity2: 多詞比對（忠實移植 Picard 演算法） ----
double similarity2(const QString& a, const QString& b) {
    if (a.isEmpty() || b.isEmpty()) 
        return 0.0;
    if (a == b) 
        return 1.0;

    // 以 \W 分詞並濾掉空 token（與 Python: re.split(r'\W+', ...) 相同）
    static const QRegularExpression splitter(QStringLiteral("\\s+"));
    QStringList alist = a.toLower().split(splitter, Qt::SkipEmptyParts);
    QStringList blist = b.toLower().split(splitter, Qt::SkipEmptyParts);

    int alen = alist.size();
    int blen = blist.size();
    if (alen == 0 || blen == 0) 
        return 0.0;

    // 確保 alist 較短（與 Python 對齊以減少比較）
    if (alen > blen) {
        std::swap(alist, blist);
        std::swap(alen, blen);
    }

    double score = 0.0;
    // 對 alist 的每個 token，找 blist 中最佳匹配；若 ms>0.6，刪除該 blist token
    for (const QString& av : alist) {
        double ms = 0.0;
        int bestPos = -1;

        for (int pos = 0; pos < blist.size(); ++pos) {
            const double s = astrcmp(av, blist[pos]);
            if (s > ms) { ms = s; bestPos = pos; }
        }

        if (bestPos >= 0) {
            score += ms;
            if (ms > 0.6) {
                blist.removeAt(bestPos);  // 與 Python: del blist[mp]
            }
        }
    }

    // Python: return score / (alen + len(blist) * 0.4)
    const double denom = static_cast<double>(alen) + static_cast<double>(blist.size()) * 0.4;
    // alen 一開始保證 > 0
    return denom > 0.0 ? (score / denom) : 0.0;
}