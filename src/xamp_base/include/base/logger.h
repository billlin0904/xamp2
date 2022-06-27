//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/fmt/ostr.h>

#include <base/singleton.h>
#include <base/base.h>
#include <base/memory.h>

namespace xamp::base {

struct XAMP_BASE_API AutoRegisterLoggerName {
    AutoRegisterLoggerName(std::string_view s);

    operator std::string_view() const {
        return GetLoggerName();
    }

    operator std::string() const {
        return GetLoggerName().data();
    }

    friend bool operator==(const AutoRegisterLoggerName &a, const std::string &b) {
        const std::string s = a;
        return s == b;
    }

    friend bool operator!=(const AutoRegisterLoggerName& a, const std::string& b) {
        const std::string s = a;
        return s != b;
    }

    std::string_view GetLoggerName() const;
    size_t index;
};

#define DECLARE_LOG_NAME(LogName) inline const AutoRegisterLoggerName k##LogName##LoggerName(#LogName)

DECLARE_LOG_NAME(Xamp);
DECLARE_LOG_NAME(WASAPIThreadPool);
DECLARE_LOG_NAME(PlaybackThreadPool);
DECLARE_LOG_NAME(BackgroundThreadPool);
DECLARE_LOG_NAME(ExclusiveWasapiDevice);
DECLARE_LOG_NAME(ExclusiveWasapiDeviceType);
DECLARE_LOG_NAME(SharedWasapiDevice);
DECLARE_LOG_NAME(AsioDevice);
DECLARE_LOG_NAME(AudioPlayer);
DECLARE_LOG_NAME(VirtualMemory);
DECLARE_LOG_NAME(Soxr);
DECLARE_LOG_NAME(Compressor);
DECLARE_LOG_NAME(Volume);
DECLARE_LOG_NAME(CoreAudio);
DECLARE_LOG_NAME(DspManager);
DECLARE_LOG_NAME(FileStream);

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

    spdlog::logger* GetDefaultLogger() noexcept {
        return default_logger_.get();
    }

    std::shared_ptr<spdlog::logger> GetLogger(const std::string& name);

    const std::vector<std::string_view>& GetDefaultLoggerName();

    template <typename... Args>
    void Log(const spdlog::source_loc& loc, spdlog::level::level_enum lvl, const char* fmt, const Args &... args) {
        spdlog::log(loc, lvl, fmt, args...);
    }
private:
    Logger() = default;

	std::vector<spdlog::sink_ptr> sinks_;
	std::shared_ptr<spdlog::logger> default_logger_;
};

struct XAMP_BASE_API LoggerStream : public std::ostringstream {
    LoggerStream(const spdlog::source_loc& loc, spdlog::level::level_enum lvl, std::string_view prefix)
        : loc_(loc)
        , lvl_(lvl)
        , prefix_(prefix) {
    }

    ~LoggerStream() {
        Flush();
    }

    void Flush() {
        Logger::GetInstance().Log(loc_, lvl_, (prefix_ + str()).c_str());
    }

    spdlog::source_loc loc_;
    spdlog::level::level_enum lvl_ = spdlog::level::info;
    std::string prefix_;
};

#define XAMP_SET_LOG_LEVEL(level) xamp::base::Logger::GetInstance().GetDefaultLogger()->set_level(level)

#define XAMP_LOG_DEBUG(...) xamp::base::Logger::GetInstance().GetDefaultLogger()->debug(__VA_ARGS__)
#define XAMP_LOG_INFO(...) xamp::base::Logger::GetInstance().GetDefaultLogger()->info(__VA_ARGS__)
#define XAMP_LOG_ERROR(...) xamp::base::Logger::GetInstance().GetDefaultLogger()->error(__VA_ARGS__)
#define XAMP_LOG_TRACE(...) xamp::base::Logger::GetInstance().GetDefaultLogger()->trace(__VA_ARGS__)

#define XAMP_LOG_D(logger, ...) logger->debug(__VA_ARGS__)
#define XAMP_LOG_I(logger, ...) logger->info(__VA_ARGS__)
#define XAMP_LOG_E(logger, ...) logger->error(__VA_ARGS__)
#define XAMP_LOG_T(logger, ...) logger->trace(__VA_ARGS__)

#define XAMP_LOGS_TREACE() xamp::base::LogStream({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::trace, "")
#define XAMP_LOGS_DEBUG()  xamp::base::LogStream({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::debug, "")
#define XAMP_LOGS_INFO()   xamp::base::LogStream({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::info, "")
#define XAMP_LOGS_WARN()   xamp::base::LogStream({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::warn, "")
#define XAMP_LOGS_ERROR()  xamp::base::LogStream({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::err, "")
#define XAMP_LOGS_FATAL()  xamp::base::LogStream({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::critical, "")

}

