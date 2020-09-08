#include <sstream>
#include <filesystem>
#include <vector>

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#include <base/str_utilts.h>
#include <base/logger.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN
class DebugOutputSink : public spdlog::sinks::base_sink<std::mutex> {
public:
	DebugOutputSink() {
	}

private:
	void sink_it_(const spdlog::details::log_msg& msg) override {
		spdlog::memory_buf_t formatted;
		formatter_->format(msg, formatted);
		::OutputDebugStringW(ToStdWString(fmt::to_string(formatted)).c_str());
	}

	void flush_() override {
	}
};
#endif

static void CreateLogsDir() {
	const std::filesystem::path log_path("logs");
	
	if (!std::filesystem::exists(log_path)) {
        std::filesystem::create_directory(log_path);
	}
}

Logger & Logger::Instance() noexcept {
    static Logger logger;
    return logger;
}

void Logger::Shutdown() {
    spdlog::shutdown();
}

std::shared_ptr<spdlog::logger> Logger::GetLogger(const std::string &name) {
	auto logger = spdlog::get(name);
	if (logger != nullptr) {
		return logger;
	}

	logger = std::make_shared<spdlog::logger>(name,
		std::begin(sinks_),
		std::end(sinks_));

    logger->set_level(spdlog::level::err);
	logger->set_pattern("[%H:%M:%S.%e][%l][%n] %v");
	logger->flush_on(spdlog::level::debug);

	spdlog::register_logger(logger);

	if (name == "default") {
		default_logger_ = logger;
	}

	return logger;
}

Logger& Logger::AddDebugOutputLogger() {
#ifdef XAMP_OS_WIN
#if defined(DEBUG) || defined(_DEBUG)
	sinks_.push_back(std::make_shared<DebugOutputSink>());
#endif
#endif
	return *this;
}

Logger& Logger::AddSink(spdlog::sink_ptr sink) {
    sinks_.push_back(sink);
    return *this;
}

Logger& Logger::AddFileLogger(const std::string &file_name) {
	CreateLogsDir();

	std::ostringstream ostr;
	ostr << "logs/" << file_name;
	sinks_.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
		ostr.str(), kMaxLogFileSize, 0));
	return *this;
}

}
