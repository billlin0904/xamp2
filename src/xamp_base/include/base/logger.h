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
	static Logger& Instance();

	~Logger() = default;

	XAMP_DISABLE_COPY(Logger)

    void SetLevel(const std::string &name, spdlog::level::level_enum level);

    void Shutdown();

	Logger& AddDebugOutputLogger();

	Logger& AddFileLogger(const std::string &file_name);

    Logger& AddSink(spdlog::sink_ptr sink);

	std::shared_ptr<spdlog::logger> GetLogger(const std::string &name);

private:
	Logger() = default;

	std::vector<spdlog::sink_ptr> sinks_;
};

#define XAMP_SET_LOG_LEVEL(level) xamp::base::Logger::Instance().SetLevel("default", level)
#define XAMP_LOG_DEBUG(...) xamp::base::Logger::Instance().GetLogger("default")->debug(__VA_ARGS__)
#define XAMP_LOG_INFO(...) xamp::base::Logger::Instance().GetLogger("default")->info(__VA_ARGS__)
#define XAMP_LOG_ERROR(...) xamp::base::Logger::Instance().GetLogger("default")->error(__VA_ARGS__)
#define XAMP_LOG_TRACE(...) xamp::base::Logger::Instance().GetLogger("default")->trace(__VA_ARGS__)

}

