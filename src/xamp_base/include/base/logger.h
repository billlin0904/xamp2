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
DECLARE_LOG_NAME(SharedWasapiDevice);
DECLARE_LOG_NAME(AsioDevice);
DECLARE_LOG_NAME(AudioPlayer);
DECLARE_LOG_NAME(VirtualMemory);
DECLARE_LOG_NAME(Soxr);
DECLARE_LOG_NAME(Compressor);
DECLARE_LOG_NAME(Volume);
DECLARE_LOG_NAME(CoreAudio);
DECLARE_LOG_NAME(DspManager);

class XAMP_BASE_API Logger final {
public:
	static constexpr int kMaxLogFileSize = 1024 * 1024;

	friend class Singleton<Logger>;

	static Logger& GetInstance() noexcept;

	XAMP_DISABLE_COPY(Logger)

	Logger& Startup();

    void Shutdown();

	Logger& AddDebugOutputLogger();

	Logger& AddFileLogger(const std::string &file_name);

    Logger& AddSink(spdlog::sink_ptr sink);

	Logger& AddConsoleLogger();

    spdlog::logger* GetDefaultLogger() noexcept {
        return default_logger_.get();
	}

	std::shared_ptr<spdlog::logger> GetLogger(const std::string &name);

    const std::vector<std::string_view> & GetDefaultLoggerName();

private:
	Logger() = default;

	std::vector<spdlog::sink_ptr> sinks_;
	std::shared_ptr<spdlog::logger> default_logger_;
};

#define XAMP_SET_LOG_LEVEL(level) xamp::base::Logger::GetInstance().GetDefaultLogger()->set_level(level)
#define XAMP_LOG_DEBUG(...) xamp::base::Logger::GetInstance().GetDefaultLogger()->debug(__VA_ARGS__)
#define XAMP_LOG_INFO(...) xamp::base::Logger::GetInstance().GetDefaultLogger()->info(__VA_ARGS__)
#define XAMP_LOG_ERROR(...) xamp::base::Logger::GetInstance().GetDefaultLogger()->error(__VA_ARGS__)
#define XAMP_LOG_TRACE(...) xamp::base::Logger::GetInstance().GetDefaultLogger()->trace(__VA_ARGS__)

#define XAMP_LOG_D(logger, ...) logger->debug(__VA_ARGS__)
#define XAMP_LOG_I(logger, ...) logger->info(__VA_ARGS__)

}

