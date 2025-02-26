//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/shared_singleton.h>

#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

#include <memory>
#include <string_view>

XAMP_BASE_NAMESPACE_BEGIN

class Logger;

using LoggerPtr = std::shared_ptr<Logger>;

#define XAMP_DECLARE_LOG_NAME(LogName) inline constexpr std::string_view k##LogName##LoggerName(#LogName)
#define XAMP_LOG_NAME(LogName) k##LogName##LoggerName

XAMP_DECLARE_LOG_NAME(Xamp);
XAMP_DECLARE_LOG_NAME(CoreAudio);
	
XAMP_BASE_NAMESPACE_END

#ifdef XAMP_OS_WIN
template <typename T, size_t S>
inline constexpr size_t __get_file_name_offset(const T(&str)[S], size_t i = S - 1) noexcept {
    return (str[i] == '/' || str[i] == '\\') ? i + 1 : (i > 0 ? __get_file_name_offset(str, i - 1) : 0);
}

template <typename T>
inline constexpr size_t __get_file_name_offset(T(&)[1]) noexcept {
    return 0;
}

#define __FILENAME__ &__FILE__[__get_file_name_offset(__FILE__)]
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

#define XampLoggerFactory xamp::base::SharedSingleton<xamp::base::LoggerManager>::GetInstance()

template<typename... Args>
std::string make_constexpr_string(const std::string& format, Args... args) {
    return fmt::format(format, args...);
}

#define XAMP_LOG(Level, ...) xamp::base::SharedSingleton<xamp::base::LoggerManager>::GetInstance().GetDefaultLogger()->Log(Level, CurrentLocation, __VA_ARGS__)
#define XAMP_LOG_DEBUG(...)    XAMP_LOG(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define XAMP_LOG_INFO(...)     XAMP_LOG(LOG_LEVEL_INFO,  __VA_ARGS__)
#define XAMP_LOG_ERROR(...)    XAMP_LOG(LOG_LEVEL_ERROR, __VA_ARGS__)
#define XAMP_LOG_TRACE(...)    XAMP_LOG(LOG_LEVEL_TRACE, __VA_ARGS__)
#define XAMP_LOG_WARN(...)     XAMP_LOG(LOG_LEVEL_WARN,  __VA_ARGS__)
#define XAMP_LOG_CRITICAL(...) XAMP_LOG(LOG_LEVEL_TRACE, __VA_ARGS__)

#define XAMP_LOG_LEVEL(logger, Level, ...) logger->Log(Level, CurrentLocation, __VA_ARGS__)
#define XAMP_LOG_D(logger, ...) XAMP_LOG_LEVEL(logger, LOG_LEVEL_DEBUG, __VA_ARGS__)
#define XAMP_LOG_I(logger, ...) XAMP_LOG_LEVEL(logger, LOG_LEVEL_INFO,  __VA_ARGS__)
#define XAMP_LOG_E(logger, ...) XAMP_LOG_LEVEL(logger, LOG_LEVEL_ERROR, __VA_ARGS__)
#define XAMP_LOG_T(logger, ...) XAMP_LOG_LEVEL(logger, LOG_LEVEL_TRACE, __VA_ARGS__)
#define XAMP_LOG_W(logger, ...) XAMP_LOG_LEVEL(logger, LOG_LEVEL_WARN,  __VA_ARGS__)
#define XAMP_LOG_C(logger, ...) XAMP_LOG_LEVEL(logger, LOG_LEVEL_TRACE, __VA_ARGS__)
