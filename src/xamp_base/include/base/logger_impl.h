//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <string>

#include <base/singleton.h>
#include <base/stl.h>
#include <base/base.h>
#include <base/logger.h>

#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/sink.h>

namespace spdlog {
    class logger;

    namespace sinks {
        class sink;
    }
}

namespace xamp::base {

enum LogLevel {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL,
    LOG_LEVEL_OFF,
};

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

class XAMP_BASE_API LoggerWriter {
public:
    explicit LoggerWriter(const std::shared_ptr<spdlog::logger>& logger);

    DECLARE_LOG_ARG_API(Trace, LOG_LEVEL_TRACE)
    DECLARE_LOG_ARG_API(Error, LOG_LEVEL_ERROR)
	DECLARE_LOG_ARG_API(Debug, LOG_LEVEL_DEBUG)
    DECLARE_LOG_ARG_API(Info, LOG_LEVEL_INFO)

    void SetLevel(LogLevel level);

private:
    template <typename... Args>
    void Log(LogLevel level, const char *filename, int32_t line, const char* func, std::string_view s, const Args&... args) {
        auto message = fmt::format(s, args...);
        LogMsg(level, filename, line, func, message);
    }

    void LogMsg(LogLevel level, const char* filename, int32_t line, const char* func, const std::string &msg) const;

    std::shared_ptr<spdlog::logger> logger_;
};

class XAMP_BASE_API Logger final {
public:
    static constexpr int kMaxLogFileSize = 1024 * 1024;

    friend class Singleton<Logger>;

    static Logger& GetInstance() noexcept;

    XAMP_DISABLE_COPY(Logger)

	Logger& Startup();

    void Shutdown();

    Logger& AddDebugOutputLogger();

    Logger& AddFileLogger(const std::string& file_name);

    Logger& AddSink(spdlog::sink_ptr sink);

    Logger& AddConsoleLogger();

    LoggerWriter* GetDefaultLogger() noexcept {
        return default_logger_.get();
    }

    std::shared_ptr<LoggerWriter> GetLogger(const std::string& name);

    const Vector<std::string_view>& GetDefaultLoggerName();

    void SetLevel(LogLevel level);

private:
    Logger() = default;

    Vector<spdlog::sink_ptr> sinks_;
    std::shared_ptr<LoggerWriter> default_logger_;
};

}

