//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

namespace xamp::base {

class LoggerWriter;
	
}

#define XAMP_SET_LOG_LEVEL(level) xamp::base::Logger::GetInstance().SetLevel(level)

#define XAMP_LOG_DEBUG(...) xamp::base::Logger::GetInstance().GetDefaultLogger()->LogDebug(__VA_ARGS__)
#define XAMP_LOG_INFO(...) xamp::base::Logger::GetInstance().GetDefaultLogger()->LogInfo(__VA_ARGS__)
#define XAMP_LOG_ERROR(...) xamp::base::Logger::GetInstance().GetDefaultLogger()->LogError(__VA_ARGS__)
#define XAMP_LOG_TRACE(...) xamp::base::Logger::GetInstance().GetDefaultLogger()->LogTrace(__VA_ARGS__)

#define XAMP_LOG_D(logger, ...) logger->LogDebug(__VA_ARGS__)
#define XAMP_LOG_I(logger, ...) logger->LogInfo(__VA_ARGS__)
#define XAMP_LOG_E(logger, ...) logger->LogError(__VA_ARGS__)
#define XAMP_LOG_T(logger, ...) logger->LogTrace(__VA_ARGS__)