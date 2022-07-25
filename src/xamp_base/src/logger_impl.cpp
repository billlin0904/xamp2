#include <sstream>
#include <filesystem>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>

#include <base/platform.h>
#include <base/fs.h>
#include <base/fastmutex.h>
#include <base/str_utilts.h>
#include <base/logger_impl.h>

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

LoggerManager::LoggerManager() noexcept {
	default_logger_ = GetLogger(kXampLoggerName);
}

const Vector<std::string_view> & LoggerManager::GetWellKnownName() {
    return Singleton<Vector<std::string_view>>::GetInstance();
}

void LoggerManager::SetLevel(LogLevel level) {
	default_logger_->SetLevel(level);
}

LoggerManager & LoggerManager::GetInstance() noexcept {    
    return Singleton<LoggerManager>::GetInstance();
}

LoggerManager& LoggerManager::Startup() {
	default_logger_->LogDebug("{}", "=:==:==:==:==:= LoggerManager init success. =:==:==:==:==:=");
	return *this;
}

void LoggerManager::Shutdown() {
	default_logger_->LogDebug("=:==:==:==:==:= LoggerManager shutdown =:==:==:==:==:=");
	spdlog::shutdown();
}

Logger::Logger(const std::shared_ptr<spdlog::logger>& logger)
	: logger_(logger) {
}

void Logger::LogMsg(LogLevel level, const char* filename, int32_t line, const char* func, const std::string& msg) const {
	logger_->log(
		spdlog::source_loc{ filename, line, func },
		static_cast<spdlog::level::level_enum>(level),
		msg);
}

void Logger::SetLevel(LogLevel level) {
	logger_->set_level(static_cast<spdlog::level::level_enum>(level));
}

const std::string& Logger::GetName() const {
	return logger_->name();
}

std::shared_ptr<Logger> LoggerManager::GetLogger(const std::string &name) {
	if (default_logger_ != nullptr) {
		if (name == default_logger_->GetName()) {
			return default_logger_;
		}
	}	

	auto logger = spdlog::get(name);
	if (logger != nullptr) {
		return std::make_shared<Logger>(logger);
	}

	logger = std::make_shared<spdlog::logger>(name,
		std::begin(sinks_),
		std::end(sinks_));

    logger->set_level(spdlog::level::debug);
	logger->set_pattern("[%H:%M:%S.%e][%l][%n][%t] %^%v%$");
	logger->flush_on(spdlog::level::debug);

	spdlog::register_logger(logger);
	return std::make_shared<Logger>(logger);
}

LoggerManager& LoggerManager::AddConsole() {
#ifdef XAMP_OS_WIN
	sinks_.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
#else
#endif
	return *this;
}

LoggerManager& LoggerManager::AddDebugOutput() {
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

LoggerManager& LoggerManager::AddSink(spdlog::sink_ptr sink) {
    sinks_.push_back(sink);
    return *this;
}

LoggerManager& LoggerManager::AddLogFile(const std::string &file_name) {
	CreateLogsDir();

	std::ostringstream ostr;
	ostr << "logs/" << file_name;
	sinks_.push_back(std::make_shared<spdlog::sinks::rotating_file_sink<FastMutex>>(
		ostr.str(), kMaxLogFileSize, 0));
	return *this;
}

}
