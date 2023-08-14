//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
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

#define XAMP_DECLARE_LOG_NAME(LogName) inline const std::string_view k##LogName##LoggerName(#LogName)

XAMP_DECLARE_LOG_NAME(Xamp);
XAMP_DECLARE_LOG_NAME(CoreAudio);
	
XAMP_BASE_NAMESPACE_END

#define XAM_LOG_MANAGER() xamp::base::LoggerManager::GetInstance()

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