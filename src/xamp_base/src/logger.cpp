#include <sstream>
#include <filesystem>
#include <vector>

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>

#include <base/platform.h>
#include <base/fastmutex.h>
#include <base/str_utilts.h>
#include <base/logger.h>

namespace xamp::base {

AutoRegisterLoggerName::AutoRegisterLoggerName(std::string_view s)
	: index(Singleton<Vector<std::string_view>>::GetInstance().size()) {
	Singleton<Vector<std::string_view>>::GetInstance().push_back(s);
}

std::string_view AutoRegisterLoggerName::GetLoggerName() const {
    return Singleton<Vector<std::string_view>>::GetInstance()[index];
}

static void CreateLogsDir() {
	const Path log_path("logs");
	
	if (!Fs::exists(log_path)) {
        Fs::create_directory(log_path);
	}
}

const Vector<std::string_view> & Logger::GetDefaultLoggerName() {
    return Singleton<Vector<std::string_view>>::GetInstance();
}

Logger & Logger::GetInstance() noexcept {    
    return Singleton<Logger>::GetInstance();
}

Logger& Logger::Startup() {
    GetLogger(kXampLoggerName);

	if (default_logger_ != nullptr) {
		default_logger_->debug("=:==:==:==:==:= Logger init success. =:==:==:==:==:=");
	}
	return *this;
}

void Logger::Shutdown() {
	if (default_logger_ != nullptr) {
		default_logger_->debug("=:==:==:==:==:= Logger shutdwon =:==:==:==:==:=");
	}
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

    logger->set_level(spdlog::level::debug);
	logger->set_pattern("[%H:%M:%S.%e][%l][%n][%t] %^%v%$");
	logger->flush_on(spdlog::level::debug);

	spdlog::register_logger(logger);

    if (kXampLoggerName == name) {
		default_logger_ = logger;
	}

	return logger;
}

Logger& Logger::AddConsoleLogger() {
#ifdef XAMP_OS_WIN
	sinks_.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
#else
#endif
	return *this;
}

Logger& Logger::AddDebugOutputLogger() {
#ifdef XAMP_OS_WIN
	// OutputDebugString 會產生例外導致AddVectoredExceptionHandler註冊的
	// Handler會遞迴的呼叫下去, 所以只有在除錯模式下才使用.
	// https://stackoverflow.com/questions/25634376/why-does-addvectoredexceptionhandler-crash-my-dll
	if (IsDebuging()) {
		sinks_.push_back(std::make_shared<spdlog::sinks::msvc_sink<FastMutex>>());
	}
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
	sinks_.push_back(std::make_shared<spdlog::sinks::rotating_file_sink<FastMutex>>(
		ostr.str(), kMaxLogFileSize, 0));
	return *this;
}

}
