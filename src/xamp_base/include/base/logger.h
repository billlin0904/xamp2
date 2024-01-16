//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#include <memory>
#include <string_view>
#include <string>

XAMP_BASE_NAMESPACE_BEGIN

class Logger;

using LoggerPtr = std::shared_ptr<Logger>;

#define XAMP_DECLARE_LOG_NAME(LogName) inline constexpr std::string_view k##LogName##LoggerName(#LogName)

XAMP_DECLARE_LOG_NAME(Xamp);
XAMP_DECLARE_LOG_NAME(CoreAudio);
	
XAMP_BASE_NAMESPACE_END

constexpr auto* GetFileName(const char* const path) {
    const auto* start = path;
    for (const auto* ch = path; *ch != '\0'; ++ch) {
        if (*ch == '\\' || *ch == '/') {
            start = ch;
        }
    }
    if (start != path) {
        ++start;
    }
    return start;
}

struct XAMP_BASE_API SourceLocation {
    const char* file;
    int line;
    const char* function;

    constexpr SourceLocation(const char* file, int line, const char* function)
        : file(file)
        , line(line)
        , function(function) {
    }
};

#define CurrentLocation \
    SourceLocation { GetFileName(__FILE__), __LINE__, __func__ }

#define XAM_LOG_MANAGER() xamp::base::LoggerManager::GetInstance()

#define XAMP_LOG(Level, Format, ...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->Log(Level, CurrentLocation, Format, __VA_ARGS__)
#define XAMP_LOG_DEBUG(...)    XAMP_LOG(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define XAMP_LOG_INFO(...)     XAMP_LOG(LOG_LEVEL_INFO,  __VA_ARGS__)
#define XAMP_LOG_ERROR(...)    XAMP_LOG(LOG_LEVEL_ERROR, __VA_ARGS__)
#define XAMP_LOG_TRACE(...)    XAMP_LOG(LOG_LEVEL_TRACE, __VA_ARGS__)
#define XAMP_LOG_WARN(...)     XAMP_LOG(LOG_LEVEL_WARN,  __VA_ARGS__)
#define XAMP_LOG_CRITICAL(...) XAMP_LOG(LOG_LEVEL_TRACE, __VA_ARGS__)

#define XAMP_LOG_LEVEL(logger, Level, Format, ...) logger->Log(Level, CurrentLocation, Format, __VA_ARGS__)
#define XAMP_LOG_D(logger, ...) XAMP_LOG_LEVEL(logger, LOG_LEVEL_DEBUG, __VA_ARGS__)
#define XAMP_LOG_I(logger, ...) XAMP_LOG_LEVEL(logger, LOG_LEVEL_INFO,  __VA_ARGS__)
#define XAMP_LOG_E(logger, ...) XAMP_LOG_LEVEL(logger, LOG_LEVEL_ERROR, __VA_ARGS__)
#define XAMP_LOG_T(logger, ...) XAMP_LOG_LEVEL(logger, LOG_LEVEL_TRACE, __VA_ARGS__)
#define XAMP_LOG_W(logger, ...) XAMP_LOG_LEVEL(logger, LOG_LEVEL_WARN,  __VA_ARGS__)
#define XAMP_LOG_C(logger, ...) XAMP_LOG_LEVEL(logger, LOG_LEVEL_TRACE, __VA_ARGS__)