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

#define XAMP_DECLARE_LOG_NAME(LogName) inline const std::string_view k##LogName##LoggerName(#LogName)

XAMP_DECLARE_LOG_NAME(Xamp);
XAMP_DECLARE_LOG_NAME(CoreAudio);
	
}

#define XAMP_DEFAULT_LOG() xamp::base::LoggerManager::GetInstance()

#define XAMP_LOG(Level, Format, ...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->Log(Level, __FILE__, __LINE__, __FUNCTION__, Format, __VA_ARGS__)
#define XAMP_LOG_DEBUG(...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->LogDebug(__VA_ARGS__)
#define XAMP_LOG_INFO(...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->LogInfo(__VA_ARGS__)
#define XAMP_LOG_ERROR(...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->LogError(__VA_ARGS__)
#define XAMP_LOG_TRACE(...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->LogTrace(__VA_ARGS__)
#define XAMP_LOG_WARN(...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->LogWarn(__VA_ARGS__)
#define XAMP_LOG_CRITICAL(...) xamp::base::LoggerManager::GetInstance().GetDefaultLogger()->LogCritical(__VA_ARGS__)

#define XAMP_LOG_LEVEL(logger, Level, Format, ...) logger->Log(Level, __FILE__, __LINE__, __FUNCTION__, Format, __VA_ARGS__)
#define XAMP_LOG_D(logger, ...) logger->LogDebug(__VA_ARGS__)
#define XAMP_LOG_I(logger, ...) logger->LogInfo(__VA_ARGS__)
#define XAMP_LOG_E(logger, ...) logger->LogError(__VA_ARGS__)
#define XAMP_LOG_T(logger, ...) logger->LogTrace(__VA_ARGS__)
#define XAMP_LOG_W(logger, ...) logger->LogWarn(__VA_ARGS__)
#define XAMP_LOG_C(logger, ...) logger->LogCritical(__VA_ARGS__)