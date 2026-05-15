//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/fastmutex.h>
#include <base/shared_singleton.h>

#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace spdlog {
    class logger;

    namespace sinks {
        class sink;
    }

    using sink_ptr = std::shared_ptr<sinks::sink>;
}

#ifdef XAMP_OS_WIN
template <typename T, size_t S>
XAMP_ALWAYS_INLINE constexpr size_t compiler_time_get_file_name_offset(const T(&str)[S], size_t i = S - 1) {
    return (str[i] == '/' || str[i] == '\\')
        ? i + 1
        : (i > 0 ? compiler_time_get_file_name_offset(str, i - 1) : 0);
}

template <typename T>
XAMP_ALWAYS_INLINE constexpr size_t compiler_time_get_file_name_offset(T(&)[1]) {
    return 0;
}

#define __FILENAME__ &__FILE__[compiler_time_get_file_name_offset(__FILE__)]
#endif

#ifndef XAMP_OS_WIN
#define __FILENAME__ __FILE_NAME__
#define __FUNCTION__ __func__
#endif

#ifdef __cpp_lib_source_location

#include <source_location>
using SourceLocation = std::source_location;

#define CurrentLocation SourceLocation::current()
#else

struct XAMP_BASE_API SourceLocation {
    uint32_t line_ = 0;
    uint32_t column_ = 0;
    const char* file_name_ = "";
    const char* function_name_ = "";

    constexpr SourceLocation() = default;

    constexpr SourceLocation(uint32_t line, uint32_t column, const char* file_name, const char* function_name)
        : line_(line)
        , column_(column)
        , file_name_(file_name)
        , function_name_(function_name) {
    }

    static SourceLocation current() {
        return SourceLocation();
    }

    uint32_t line() const {
        return this->line_;
    }

    uint32_t column() const {
        return this->column_;
    }

    const char* file_name() const {
        return this->file_name_;
    }

    const char* function_name() const {
        return this->function_name_;
    }
};

#define CurrentLocation SourceLocation { __LINE__, 0, __FILENAME__, __FUNCTION__ }
#endif

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

class Logger;

using LoggerPtr = std::shared_ptr<Logger>;
using LoggerMutex = std::recursive_mutex;

#define XAMP_DECLARE_LOG_NAME(LogName) inline constexpr std::string_view k##LogName##LoggerName(#LogName)
#define XAMP_LOG_NAME(LogName) k##LogName##LoggerName
#define XAMP_LOG_CREATE_LOGGER(LogName) XampLoggerFactory.GetLogger(#LogName)

XAMP_DECLARE_LOG_NAME(Xamp);
XAMP_DECLARE_LOG_NAME(CoreAudio);

class XAMP_BASE_API Logger {
public:
    explicit Logger(const std::shared_ptr<spdlog::logger>& logger);

    template <typename Args>
    void LogTrace(Args&& args) {
        Log(LOG_LEVEL_TRACE, nullptr, 0, nullptr, "{}", std::forward<Args>(args));
    }

    template <typename... Args>
    void LogTrace(std::string_view s, Args&&... args) {
        Log(LOG_LEVEL_TRACE, nullptr, 0, nullptr, s, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void LogTrace(const char* filename, int32_t line, const char* func, std::string_view s, Args&&... args) {
        Log(LOG_LEVEL_TRACE, filename, line, func, s, std::forward<Args>(args)...);
    }

    template <typename Args>
    void LogDebug(Args&& args) {
        Log(LOG_LEVEL_DEBUG, nullptr, 0, nullptr, "{}", std::forward<Args>(args));
    }

    template <typename... Args>
    void LogDebug(std::string_view s, Args&&... args) {
        Log(LOG_LEVEL_DEBUG, nullptr, 0, nullptr, s, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void LogDebug(const char* filename, int32_t line, const char* func, std::string_view s, Args&&... args) {
        Log(LOG_LEVEL_DEBUG, filename, line, func, s, std::forward<Args>(args)...);
    }

    template <typename Args>
    void LogInfo(Args&& args) {
        Log(LOG_LEVEL_INFO, nullptr, 0, nullptr, "{}", std::forward<Args>(args));
    }

    template <typename... Args>
    void LogInfo(std::string_view s, Args&&... args) {
        Log(LOG_LEVEL_INFO, nullptr, 0, nullptr, s, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void LogInfo(const char* filename, int32_t line, const char* func, std::string_view s, Args&&... args) {
        Log(LOG_LEVEL_INFO, filename, line, func, s, std::forward<Args>(args)...);
    }

    template <typename Args>
    void LogWarn(Args&& args) {
        Log(LOG_LEVEL_WARN, nullptr, 0, nullptr, "{}", std::forward<Args>(args));
    }

    template <typename... Args>
    void LogWarn(std::string_view s, Args&&... args) {
        Log(LOG_LEVEL_WARN, nullptr, 0, nullptr, s, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void LogWarn(const char* filename, int32_t line, const char* func, std::string_view s, Args&&... args) {
        Log(LOG_LEVEL_WARN, filename, line, func, s, std::forward<Args>(args)...);
    }

    template <typename Args>
    void LogError(Args&& args) {
        Log(LOG_LEVEL_ERROR, nullptr, 0, nullptr, "{}", std::forward<Args>(args));
    }

    template <typename... Args>
    void LogError(std::string_view s, Args&&... args) {
        Log(LOG_LEVEL_ERROR, nullptr, 0, nullptr, s, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void LogError(const char* filename, int32_t line, const char* func, std::string_view s, Args&&... args) {
        Log(LOG_LEVEL_ERROR, filename, line, func, s, std::forward<Args>(args)...);
    }

    template <typename Args>
    void LogCritical(Args&& args) {
        Log(LOG_LEVEL_CRITICAL, nullptr, 0, nullptr, "{}", std::forward<Args>(args));
    }

    template <typename... Args>
    void LogCritical(std::string_view s, Args&&... args) {
        Log(LOG_LEVEL_CRITICAL, nullptr, 0, nullptr, s, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void LogCritical(const char* filename, int32_t line, const char* func, std::string_view s, Args&&... args) {
        Log(LOG_LEVEL_CRITICAL, filename, line, func, s, std::forward<Args>(args)...);
    }

    void SetLevel(LogLevel level);

    [[nodiscard]] LogLevel GetLevel() const;

    [[nodiscard]] const std::string& GetName() const;

    [[nodiscard]] bool ShouldLog(LogLevel level) const;

    template <typename T>
    void Log(LogLevel level, const SourceLocation& source_location, const T& message) {
        if (!ShouldLog(level)) {
            return;
        }
        LogMsg(level, source_location.file_name(), source_location.line(), source_location.function_name(), message);
    }

    template <typename... Args>
    void Log(LogLevel level, const SourceLocation& source_location, fmt::format_string<Args...> s, Args&&... args) {
        if (!ShouldLog(level)) {
            return;
        }
        auto message = fmt::format(s, std::forward<Args>(args)...);
        LogMsg(level, source_location.file_name(), source_location.line(), source_location.function_name(), message);
    }

    template <typename... Args>
    void Log(LogLevel level, const char* filename, int32_t line, const char* func, fmt::format_string<Args...> s, Args&&... args) {
        if (!ShouldLog(level)) {
            return;
        }
        auto message = fmt::format(s, std::forward<Args>(args)...);
        LogMsg(level, filename, line, func, message);
    }

private:
    void LogMsg(LogLevel level, const char* filename, int32_t line, const char* func, const std::string& msg) const;

    std::shared_ptr<spdlog::logger> logger_;
};

class XAMP_BASE_API LoggerManager final {
public:
    static constexpr int kMaxLogFileSize = 1024 * 1024;

    XAMP_DECLARE_SINGLETON_NAME()

    LoggerManager();

    ~LoggerManager();

    XAMP_DISABLE_COPY(LoggerManager)

    LoggerManager& Startup();

    LoggerManager& AddDebugOutput();

    LoggerManager& AddLogFile(const std::string& file_name);

    LoggerManager& AddSink(spdlog::sink_ptr sink);

    XAMP_CHECK_LIFETIME [[nodiscard]] Logger* GetDefaultLogger() const {
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

#define XampLoggerFactory xamp::base::SharedSingleton<xamp::base::LoggerManager>::GetInstance()

#define XAMP_LOG(Level, ...) xamp::base::SharedSingleton<xamp::base::LoggerManager>::GetInstance().GetDefaultLogger()->Log(Level, CurrentLocation, __VA_ARGS__)
#define XAMP_LOG_DEBUG(...)    XAMP_LOG(xamp::base::LOG_LEVEL_DEBUG, __VA_ARGS__)
#define XAMP_LOG_INFO(...)     XAMP_LOG(xamp::base::LOG_LEVEL_INFO,  __VA_ARGS__)
#define XAMP_LOG_ERROR(...)    XAMP_LOG(xamp::base::LOG_LEVEL_ERROR, __VA_ARGS__)
#define XAMP_LOG_TRACE(...)    XAMP_LOG(xamp::base::LOG_LEVEL_TRACE, __VA_ARGS__)
#define XAMP_LOG_WARN(...)     XAMP_LOG(xamp::base::LOG_LEVEL_WARN,  __VA_ARGS__)
#define XAMP_LOG_CRITICAL(...) XAMP_LOG(xamp::base::LOG_LEVEL_CRITICAL, __VA_ARGS__)

#define XAMP_LOG_LEVEL(logger, Level, ...) logger->Log(Level, CurrentLocation, __VA_ARGS__)
#define XAMP_LOG_D(logger, ...) XAMP_LOG_LEVEL(logger, xamp::base::LOG_LEVEL_DEBUG, __VA_ARGS__)
#define XAMP_LOG_I(logger, ...) XAMP_LOG_LEVEL(logger, xamp::base::LOG_LEVEL_INFO,  __VA_ARGS__)
#define XAMP_LOG_E(logger, ...) XAMP_LOG_LEVEL(logger, xamp::base::LOG_LEVEL_ERROR, __VA_ARGS__)
#define XAMP_LOG_T(logger, ...) XAMP_LOG_LEVEL(logger, xamp::base::LOG_LEVEL_TRACE, __VA_ARGS__)
#define XAMP_LOG_W(logger, ...) XAMP_LOG_LEVEL(logger, xamp::base::LOG_LEVEL_WARN,  __VA_ARGS__)
#define XAMP_LOG_C(logger, ...) XAMP_LOG_LEVEL(logger, xamp::base::LOG_LEVEL_CRITICAL, __VA_ARGS__)
