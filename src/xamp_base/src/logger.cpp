#include <sstream>
#include <filesystem>
#include <vector>

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#include <base/platform.h>
#include <base/fastmutex.h>
#include <base/str_utilts.h>
#include <base/logger.h>

namespace xamp::base {

const char kDefaultLoggerName[] = "xamp";
const char kWASAPIThreadPoolLoggerName[] = "WASAPIThreadPool";
const char kPlaybackThreadPoolLoggerName[] = "PlaybackThreadPool";
const char kBackgroundThreadPoolLoggerName[] = "BackgroundThreadPool";
const char kExclusiveWasapiDeviceLoggerName[] = "ExclusiveWasapiDevice";
const char kSharedWasapiDeviceLoggerName[] = "SharedWasapiDevice";
const char kAsioDeviceLoggerName[] = "AsioDevice";
const char kAudioPlayerLoggerName[] = "AudioPlayer";
const char kVirtualMemoryLoggerName[] = "VirtualMemory";
const char kSoxrLoggerName[] = "Soxr";
const char kCompressorLoggerName[] = "Compressor";
const char kVolumeLoggerName[] = "Volume";
const char kCoreAudioLoggerName[] = "CoreAudio";
const char kDspManagerLoggerName[] = "DspManager";

#ifdef XAMP_OS_WIN
class DebugOutputSink : public spdlog::sinks::base_sink<FastMutex> {
public:
	DebugOutputSink() {
	}

private:
	void sink_it_(const spdlog::details::log_msg& msg) override {
		spdlog::memory_buf_t formatted;
		formatter_->format(msg, formatted);
		::OutputDebugStringW(String::ToStdWString(fmt::to_string(formatted)).c_str());
	}

	void flush_() override {
	}
};
#endif

static void CreateLogsDir() {
	const Path log_path("logs");
	
	if (!Fs::exists(log_path)) {
        Fs::create_directory(log_path);
	}
}

Logger & Logger::GetInstance() noexcept {    
    return Singleton<Logger>::GetInstance();
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

    logger->set_level(spdlog::level::debug);
	logger->set_pattern("[%H:%M:%S.%e][%l][%n][%t] %^%v%$");
	logger->flush_on(spdlog::level::debug);

	spdlog::register_logger(logger);

	if (name == kDefaultLoggerName) {
		default_logger_ = logger;
	}

	return logger;
}

Logger& Logger::AddConsoleLogger() {
#ifdef XAMP_OS_WIN
	sinks_.push_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
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
		sinks_.push_back(std::make_shared<DebugOutputSink>());
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
