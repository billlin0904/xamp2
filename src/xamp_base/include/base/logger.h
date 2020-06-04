//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/fmt/ostr.h>

#include <base/base.h>
#include <base/memory.h>

namespace xamp::base {

class XAMP_BASE_API Logger final {
public:
	static Logger& Instance() noexcept;

	XAMP_DISABLE_COPY(Logger)

    void Shutdown();

	Logger& AddDebugOutputLogger();

	Logger& AddFileLogger(const std::string &file_name);

    Logger& AddSink(spdlog::sink_ptr sink);

	std::shared_ptr<spdlog::logger>& GetDefaultLogger() noexcept {
		return default_logger_;
	}

	std::shared_ptr<spdlog::logger> GetLogger(const std::string &name);

private:
	Logger() = default;

	std::vector<spdlog::sink_ptr> sinks_;
	std::shared_ptr<spdlog::logger> default_logger_;
};

#define XAMP_SET_LOG_LEVEL(level) xamp::base::Logger::Instance().GetDefaultLogger()->set_level(level)
#define XAMP_LOG_DEBUG(...) xamp::base::Logger::Instance().GetDefaultLogger()->debug(__VA_ARGS__)
#define XAMP_LOG_INFO(...) xamp::base::Logger::Instance().GetDefaultLogger()->info(__VA_ARGS__)
#define XAMP_LOG_ERROR(...) xamp::base::Logger::Instance().GetDefaultLogger()->error(__VA_ARGS__)
#define XAMP_LOG_TRACE(...) xamp::base::Logger::Instance().GetDefaultLogger()->trace(__VA_ARGS__)

}

