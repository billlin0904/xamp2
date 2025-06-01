//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/singleton.h>
#include <base/stl.h>
#include <base/logger.h>

#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

#include <memory>
#include <mutex>
#include <string>

#include "fastmutex.h"

namespace spdlog {
    class logger;

    namespace sinks {
        class sink;
    }

    using sink_ptr = std::shared_ptr<sinks::sink>;
}

XAMP_BASE_NAMESPACE_BEGIN

enum LogLevel {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL,
    LOG_LEVEL_OFF,
};

using LoggerMutex = std::recursive_mutex;

#define DECLARE_LOG_ARG_API(Name, Level) \
    template <typename Args>\
    void Log##Name(Args &&args) {\
		Log(Level, nullptr, 0, nullptr, "{}", std::forward<Args>(args));\
	}\
    template <typename... Args>\
	void Log##Name(std::string_view s, Args &&...args) {\
		Log(Level, nullptr, 0, nullptr, s, std::forward<Args>(args)...);\
	}\
	template <typename... Args>\
	void Log##Name(const char* filename, int32_t line, const char* func, std::string_view s, Args &&...args) {\
		Log(Level, filename, line, func, s, std::forward<Args>(args)...);\
	}

class XAMP_BASE_API Logger {
public:
    explicit Logger(const std::shared_ptr<spdlog::logger>& logger);

    DECLARE_LOG_ARG_API(Trace, LOG_LEVEL_TRACE)
	DECLARE_LOG_ARG_API(Debug, LOG_LEVEL_DEBUG)
	DECLARE_LOG_ARG_API(Info, LOG_LEVEL_INFO)
	DECLARE_LOG_ARG_API(Warn, LOG_LEVEL_WARN)
    DECLARE_LOG_ARG_API(Error, LOG_LEVEL_ERROR)
    DECLARE_LOG_ARG_API(Critical, LOG_LEVEL_CRITICAL)

    void SetLevel(LogLevel level);

    [[nodiscard]] LogLevel GetLevel() const;

    [[nodiscard]] const std::string & GetName() const;

    [[nodiscard]] bool ShouldLog(LogLevel level) const;

    template <typename T>
    void Log(LogLevel level, const SourceLocation& source_location, const T &message) {
        if (!ShouldLog(level)) {
            return;
        }
        LogMsg(level, source_location.file_name(), source_location.line(), source_location.function_name(), message);
    }

    template <typename... Args>
    void Log(LogLevel level, const SourceLocation& source_location, fmt::format_string<Args...> s, const Args&... args) {
        if (!ShouldLog(level)) {
            return;
        }
        auto message = fmt::format(s, args...);
        LogMsg(level, source_location.file_name(), source_location.line(), source_location.function_name(), message);
    }

    template <typename... Args>
    void Log(LogLevel level, const char* filename, int32_t line, const char* func, fmt::format_string<Args...> s, const Args&... args) {
        if (!ShouldLog(level)) {
            return;
        }
        auto message = fmt::format(s, args...);
        LogMsg(level, filename, line, func, message);
    }
private:
    void LogMsg(LogLevel level, const char* filename, int32_t line, const char* func, const std::string &msg) const;

    std::shared_ptr<spdlog::logger> logger_;
};

class XAMP_BASE_API LoggerManager final {
public:
    static constexpr int kMaxLogFileSize = 1024 * 1024;

    LoggerManager() noexcept;

    ~LoggerManager();

    XAMP_DISABLE_COPY(LoggerManager)

	LoggerManager& Startup();

    LoggerManager& AddDebugOutput();

    LoggerManager& AddLogFile(const std::string& file_name);

    LoggerManager& AddSink(spdlog::sink_ptr sink);

    [[nodiscard]] Logger* GetDefaultLogger() const noexcept {
        return default_logger_.get();
    }

    LoggerPtr GetLogger(const std::string_view& name);

    std::vector<LoggerPtr> GetAllLogger();

    void SetLevel(LogLevel level);

    void Shutdown();
private:
    LoggerPtr GetLoggerImpl(const std::string& name);
    FastMutex lock_;
    std::vector<spdlog::sink_ptr> sinks_;
    LoggerPtr default_logger_;
};

XAMP_BASE_NAMESPACE_END

