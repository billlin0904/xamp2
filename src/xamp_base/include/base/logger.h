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

    operator const char *() const {
        return GetLoggerName().data();
    }

    bool operator==(const std::string &name) const {
        return std::string(*this) == name;
    }

    friend bool operator==(const AutoRegisterLoggerName &a, const std::string &b) {
        return std::string(a) == b;
    }

    std::string_view GetLoggerName() const;
    size_t index;
};

#define DECLARE_LOG_NAME(LogName) XAMP_BASE_API inline const AutoRegisterLoggerName LogName(#LogName)

#if 0
// Logger name.
extern "C" XAMP_BASE_API const char kDefaultLoggerName[];
extern "C" XAMP_BASE_API const char kWASAPIThreadPoolLoggerName[];
extern "C" XAMP_BASE_API const char kPlaybackThreadPoolLoggerName[];
extern "C" XAMP_BASE_API const char kBackgroundThreadPoolLoggerName[];
extern "C" XAMP_BASE_API const char kExclusiveWasapiDeviceLoggerName[];
extern "C" XAMP_BASE_API const char kSharedWasapiDeviceLoggerName[];
extern "C" XAMP_BASE_API const char kAsioDeviceLoggerName[];
extern "C" XAMP_BASE_API const char kAudioPlayerLoggerName[];
extern "C" XAMP_BASE_API const char kVirtualMemoryLoggerName[];
extern "C" XAMP_BASE_API const char kSoxrLoggerName[];
extern "C" XAMP_BASE_API const char kCompressorLoggerName[];
extern "C" XAMP_BASE_API const char kVolumeLoggerName[];
extern "C" XAMP_BASE_API const char kCoreAudioLoggerName[];
extern "C" XAMP_BASE_API const char kDspManagerLoggerName[];
#endif

DECLARE_LOG_NAME(kDefaultLoggerName);
DECLARE_LOG_NAME(kWASAPIThreadPoolLoggerName);
DECLARE_LOG_NAME(kPlaybackThreadPoolLoggerName);
DECLARE_LOG_NAME(kBackgroundThreadPoolLoggerName);
DECLARE_LOG_NAME(kExclusiveWasapiDeviceLoggerName);
DECLARE_LOG_NAME(kSharedWasapiDeviceLoggerName);
DECLARE_LOG_NAME(kAsioDeviceLoggerName);
DECLARE_LOG_NAME(kAudioPlayerLoggerName);
DECLARE_LOG_NAME(kVirtualMemoryLoggerName);
DECLARE_LOG_NAME(kSoxrLoggerName);
DECLARE_LOG_NAME(kCompressorLoggerName);
DECLARE_LOG_NAME(kVolumeLoggerName);
DECLARE_LOG_NAME(kCoreAudioLoggerName);
DECLARE_LOG_NAME(kDspManagerLoggerName);

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

