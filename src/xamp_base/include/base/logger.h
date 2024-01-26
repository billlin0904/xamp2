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

template <typename T, size_t S>
inline constexpr size_t __get_file_name_offset(const T(&str)[S], size_t i = S - 1) noexcept {
    return (str[i] == '/' || str[i] == '\\') ? i + 1 : (i > 0 ? __get_file_name_offset(str, i - 1) : 0);
}

template <typename T>
inline constexpr size_t __get_file_name_offset(T(&str)[1]) noexcept {
    return 0;
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

#ifdef XAMP_OS_WIN
#define __FILENAME__ &__FILE__[__get_file_name_offset(__FILE__)]
#endif

#define CurrentLocation \
    SourceLocation { __FILENAME__, __LINE__, __FUNCTION__ }

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