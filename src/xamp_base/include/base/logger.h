//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string_view>
#include <string>
#include <base/base.h>

namespace xamp::base {

class Logger;

struct XAMP_BASE_API AutoRegisterLoggerName {
    AutoRegisterLoggerName(std::string_view s);

    operator std::string_view() const {
        return GetLoggerName();
    }

    operator std::string() const {
        return GetLoggerName().data();
    }

    friend bool operator==(const AutoRegisterLoggerName& a, const std::string& b) {
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
	
}

#define XAMP_SET_LOG_LEVEL(level) xamp::base::LoggerManager::GetInstance().SetLevel(level)

#define XAMP_LOG_DEBUG(...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->LogDebug(__VA_ARGS__)
#define XAMP_LOG_INFO(...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->LogInfo(__VA_ARGS__)
#define XAMP_LOG_ERROR(...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->LogError(__VA_ARGS__)
#define XAMP_LOG_TRACE(...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->LogTrace(__VA_ARGS__)
#define XAMP_LOG_WARN(...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->LogWarn(__VA_ARGS__)
#define XAMP_LOG_CRITICAL(...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->LogCritical(__VA_ARGS__)

#define XAMP_LOG_D(logger, ...) logger->LogDebug(__VA_ARGS__)
#define XAMP_LOG_I(logger, ...) logger->LogInfo(__VA_ARGS__)
#define XAMP_LOG_E(logger, ...) logger->LogError(__VA_ARGS__)
#define XAMP_LOG_T(logger, ...) logger->LogTrace(__VA_ARGS__)
#define XAMP_LOG_W(logger, ...) logger->LogWarn(__VA_ARGS__)
#define XAMP_LOG_C(logger, ...) logger->LogCritical(__VA_ARGS__)