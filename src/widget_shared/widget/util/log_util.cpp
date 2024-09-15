#include <widget/util/log_util.h>
#include <widget/util/str_util.h>
#include <QMap>

namespace log_util {

QString getLogLevelString(LogLevel level) {
    static const QMap<LogLevel, QString> logs{
        { LogLevel::LOG_LEVEL_INFO, "info"_str },
        { LogLevel::LOG_LEVEL_DEBUG, "debug"_str },
        { LogLevel::LOG_LEVEL_TRACE, "trace"_str },
        { LogLevel::LOG_LEVEL_WARN, "warn"_str },
        { LogLevel::LOG_LEVEL_ERROR, "err"_str },
        { LogLevel::LOG_LEVEL_CRITICAL, "critical"_str },
    };
    if (!logs.contains(level)) {
        return "info"_str;
    }
    return logs[level];
}

LogLevel parseLogLevel(const QString &str) {
    static const QMap<QString, LogLevel> logs{
    	{ "info"_str, LogLevel::LOG_LEVEL_INFO},
        { "debug"_str, LogLevel::LOG_LEVEL_DEBUG},
        { "trace"_str, LogLevel::LOG_LEVEL_TRACE},
        { "warn"_str, LogLevel::LOG_LEVEL_WARN},
        { "err"_str, LogLevel::LOG_LEVEL_ERROR},
        { "critical"_str, LogLevel::LOG_LEVEL_CRITICAL},
    };
    if (!logs.contains(str)) {
        return LogLevel::LOG_LEVEL_INFO;
    }
    return logs[str];
}


}