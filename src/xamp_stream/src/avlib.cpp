#include <stream/avlib.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>

namespace xamp::stream {

XAMP_DECLARE_LOG_NAME(LibAv);

AvFormatLib::AvFormatLib() try
	: module_(LoadModule("avformat-58.dll"))
	, XAMP_LOAD_DLL_API(avformat_open_input)
	, XAMP_LOAD_DLL_API(avformat_close_input)
	, XAMP_LOAD_DLL_API(avformat_find_stream_info)
	, XAMP_LOAD_DLL_API(av_seek_frame)
	, XAMP_LOAD_DLL_API(av_read_frame)
	, XAMP_LOAD_DLL_API(avformat_network_init)
	, XAMP_LOAD_DLL_API(avformat_network_deinit) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

AvCodecLib::AvCodecLib() try
	: module_(LoadModule("avcodec-58.dll"))
	, XAMP_LOAD_DLL_API(avcodec_close)
	, XAMP_LOAD_DLL_API(avcodec_open2)
	, XAMP_LOAD_DLL_API(avcodec_find_decoder)
	, XAMP_LOAD_DLL_API(av_packet_alloc)
	, XAMP_LOAD_DLL_API(av_init_packet)
	, XAMP_LOAD_DLL_API(av_packet_unref)
	, XAMP_LOAD_DLL_API(avcodec_send_packet)
	, XAMP_LOAD_DLL_API(avcodec_receive_frame)
	, XAMP_LOAD_DLL_API(avcodec_flush_buffers)
	, XAMP_LOAD_DLL_API(av_get_bits_per_sample)
	, XAMP_LOAD_DLL_API(avcodec_find_decoder_by_name) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

AvUtilLib::AvUtilLib() try
	: module_(LoadModule("avutil-56.dll"))
	, XAMP_LOAD_DLL_API(av_free)
	, XAMP_LOAD_DLL_API(av_frame_unref)
	, XAMP_LOAD_DLL_API(av_get_bytes_per_sample)
	, XAMP_LOAD_DLL_API(av_strerror)
	, XAMP_LOAD_DLL_API(av_frame_alloc)
	, XAMP_LOAD_DLL_API(av_malloc)
	, XAMP_LOAD_DLL_API(av_samples_get_buffer_size)
	, XAMP_LOAD_DLL_API(av_log_set_callback)
	, XAMP_LOAD_DLL_API(av_log_format_line)
	, XAMP_LOAD_DLL_API(av_log_set_level)
	, XAMP_LOAD_DLL_API(av_dict_set) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

AvSwLib::AvSwLib() try
	: module_(LoadModule("swresample-3.dll"))
	, XAMP_LOAD_DLL_API(swr_free)
	, XAMP_LOAD_DLL_API(swr_alloc_set_opts)
	, XAMP_LOAD_DLL_API(swr_convert)
	, XAMP_LOAD_DLL_API(swr_init)
	, XAMP_LOAD_DLL_API(swr_close) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

static void LogPrintf(void* ptr, int level, const char* fmt, va_list vl) {
	va_list valist;
	char message[1024]{};

	va_copy(valist, vl);
	int print_prefix = 1;
	LIBAV_LIB.UtilLib->av_log_format_line(ptr, level, fmt, valist, message, sizeof(message), &print_prefix);
	va_end(valist);

	const auto message_length = strlen(message) - 1;
	if (message[message_length] == '\n') {
		message[message_length] = '\0';
	}

	auto log_level = LogLevel::LOG_LEVEL_DEBUG;

	switch (level) {
	case AV_LOG_PANIC:
	case AV_LOG_FATAL:
		log_level = LogLevel::LOG_LEVEL_ERROR;
		break;
	case AV_LOG_ERROR:
	case AV_LOG_WARNING:
		log_level = LogLevel::LOG_LEVEL_WARN;
		break;
	case AV_LOG_INFO:
	case AV_LOG_VERBOSE:
		log_level = LogLevel::LOG_LEVEL_INFO;
		break;
	case AV_LOG_DEBUG:
		log_level = LogLevel::LOG_LEVEL_DEBUG;
		break;
	default:
		return;
	}

	XAMP_LOG_LEVEL(LIBAV_LIB.logger, log_level, "{}", message);
}

AvLib::~AvLib() {
	FormatLib->avformat_network_deinit();
	XAMP_LOG_D(logger, "Network deinit.");
}

AvLib::AvLib() {
	logger = LoggerManager::GetInstance().GetLogger(kLibAvLoggerName);
	XAMP_LOG_D(logger, "Load {} success.", LIBAVCODEC_IDENT);

	FormatLib = MakeAlign<AvFormatLib>();
	CodecLib = MakeAlign<AvCodecLib>();
	SwrLib = MakeAlign<AvSwLib>();
	UtilLib = MakeAlign<AvUtilLib>();

	UtilLib->av_log_set_callback(LogPrintf);
	UtilLib->av_log_set_level(AV_LOG_FATAL);

	FormatLib->avformat_network_init();
	XAMP_LOG_D(logger, "Network init.");
}

}