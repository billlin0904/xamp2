#include <base/logger_impl.h>

#include <base/platform.h>
#include <base/fs.h>
#include <base/str_utilts.h>
#include <base/logger.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>

#include <sstream>
#include <filesystem>
#include <vector>

XAMP_BASE_NAMESPACE_BEGIN

class LogFlagFormatter final : public spdlog::custom_flag_formatter {
public:
	void format(const spdlog::details::log_msg& message, const std::tm&, spdlog::memory_buf_t& dest) override {
		const auto upper_logger_level = String::ToUpper(std::string(to_string_view(message.level).data()));
		dest.append(upper_logger_level.data(), upper_logger_level.data() + upper_logger_level.size());
	}

	[[nodiscard]] std::unique_ptr<custom_flag_formatter> clone() const override {
		return spdlog::details::make_unique<LogFlagFormatter>();
	}
};

#ifdef XAMP_OS_WIN
class DebugOutputSink : public spdlog::sinks::base_sink<LoggerMutex> {
public:
	DebugOutputSink() = default;

private:
	void sink_it_(const spdlog::details::log_msg& msg) override {
		spdlog::memory_buf_t formatted;
		formatter_->format(msg, formatted);
		// OutputDebugStringW max output 32766 (include '\0')
		::OutputDebugStringW(String::ToStdWString(fmt::to_string(formatted)).c_str());
	}

	void flush_() override {
	}
};
#endif

namespace {
	bool CreateLogsDir() {
		const Path log_path("logs");
		if (!Fs::exists(log_path)) {
			return Fs::create_directory(log_path);
		}
		return false;
	}
}

LoggerManager::LoggerManager() noexcept = default;

LoggerManager::~LoggerManager() = default;

void LoggerManager::SetLevel(LogLevel level) {
	default_logger_->SetLevel(level);
}

LoggerManager & LoggerManager::GetInstance() noexcept {    
    return Singleton<LoggerManager>::GetInstance();
}

LoggerManager& LoggerManager::Startup() {
	GetLogger(kXampLoggerName);
	default_logger_->LogDebug("{}", "<LoggerManager startup success>");
	return *this;
}

void LoggerManager::Shutdown() {
	GetLogger(kXampLoggerName);
	default_logger_->LogDebug("<LoggerManager shutdown>");
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

LogLevel Logger::GetLevel() const {
	return static_cast<LogLevel>(logger_->level());
}

const std::string& Logger::GetName() const {
	return logger_->name();
}

bool Logger::ShouldLog(LogLevel level) const {
	return logger_->should_log(static_cast<spdlog::level::level_enum>(level));
}

Vector<LoggerPtr> LoggerManager::GetAllLogger() {
	Vector<LoggerPtr> loggers;
	spdlog::details::registry::instance().apply_all([&loggers](auto x) {
		if (x->name().empty()) {
			return;
		}
		loggers.push_back(std::make_shared<Logger>(x));
		});
	return loggers;
}

LoggerPtr LoggerManager::GetLogger(const std::string_view& name) {
	return GetLogger(std::string(name));
}

LoggerPtr LoggerManager::GetLogger(const std::string &name) {
	auto logger = spdlog::get(name);
	if (logger != nullptr) {
		return std::make_shared<Logger>(logger);
	}

	logger = std::make_shared<spdlog::logger>(name,
		std::begin(sinks_),
		std::end(sinks_));

    logger->set_level(spdlog::level::info);

	auto formatter = std::make_unique<spdlog::pattern_formatter>();
#ifdef XAMP_OS_WIN
	formatter->add_flag<LogFlagFormatter>('*').set_pattern("[%H:%M:%S.%e][%*][%n][%t][%@] %v");
#else
	formatter->add_flag<LogFlagFormatter>('*').set_pattern("[%*][%n][%t][%@] %v");
#endif
	logger->set_formatter(std::move(formatter));

	logger->flush_on(spdlog::level::debug);

    if (kXampLoggerName == name) {
		default_logger_ = std::make_shared<Logger>(logger);
	}

	spdlog::register_logger(logger);
	return std::make_shared<Logger>(logger);
}

LoggerManager& LoggerManager::AddDebugOutput() {
#ifdef XAMP_OS_WIN
	// OutputDebugString 會產生例外導致AddVectoredExceptionHandler註冊的
	// Handler會遞迴的呼叫下去, 所以只有在除錯模式下才使用.
	// https://stackoverflow.com/questions/25634376/why-does-addvectoredexceptionhandler-crash-my-dll
	if (IsDebuging()) {
		sinks_.push_back(std::make_shared<DebugOutputSink>());
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
	sinks_.push_back(std::make_shared<spdlog::sinks::rotating_file_sink<LoggerMutex>>(
		ostr.str(), kMaxLogFileSize, 0));
	return *this;
}

XAMP_BASE_NAMESPACE_END
