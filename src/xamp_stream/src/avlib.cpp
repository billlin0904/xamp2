#include <set>
#include <stream/avlib.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>
#include <base/memory.h>

XAMP_STREAM_NAMESPACE_BEGIN

AvException::AvException(int32_t error)
	: Exception(Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR) 
	, error_code_(error) {
	char buf[256]{};
	LIBAV_LIB.Util->av_strerror(error, buf, sizeof(buf) - 1);
	message_.assign(buf);
}

AvException::~AvException() = default;

char const* AvException::what() const noexcept {
	return message_.c_str();
}

int32_t AvException::GetErrorCode() const noexcept {
	return error_code_;
}

XAMP_DECLARE_LOG_NAME(LibAv);

AvFormatLib::AvFormatLib() try
    : module_(OpenSharedLibrary("avformat-59"))
	, XAMP_LOAD_DLL_API(avformat_open_input)
	, XAMP_LOAD_DLL_API(avformat_close_input)
	, XAMP_LOAD_DLL_API(avformat_find_stream_info)
	, XAMP_LOAD_DLL_API(av_seek_frame)
	, XAMP_LOAD_DLL_API(av_read_frame)
	, XAMP_LOAD_DLL_API(av_write_frame)
	, XAMP_LOAD_DLL_API(avformat_write_header)
	, XAMP_LOAD_DLL_API(avformat_network_init)
	, XAMP_LOAD_DLL_API(avformat_network_deinit)
	, XAMP_LOAD_DLL_API(avformat_alloc_context)
	, XAMP_LOAD_DLL_API(avformat_new_stream)
	, XAMP_LOAD_DLL_API(avformat_query_codec)
    , XAMP_LOAD_DLL_API(av_demuxer_iterate)
	, XAMP_LOAD_DLL_API(avformat_alloc_output_context2)
	, XAMP_LOAD_DLL_API(avio_open)
	, XAMP_LOAD_DLL_API(av_interleaved_write_frame)
	, XAMP_LOAD_DLL_API(av_guess_format)
	, XAMP_LOAD_DLL_API(av_write_trailer)
	, XAMP_LOAD_DLL_API(av_dump_format)
	, XAMP_LOAD_DLL_API(avio_alloc_context)
	, XAMP_LOAD_DLL_API(avio_context_free)
	, XAMP_LOAD_DLL_API(av_find_best_stream) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

AvCodecLib::AvCodecLib() try
    : module_(OpenSharedLibrary("avcodec-59"))
	, XAMP_LOAD_DLL_API(avcodec_close)
	, XAMP_LOAD_DLL_API(avcodec_open2)
	, XAMP_LOAD_DLL_API(avcodec_alloc_context3)
	, XAMP_LOAD_DLL_API(avcodec_find_decoder)
	, XAMP_LOAD_DLL_API(av_packet_alloc)
	, XAMP_LOAD_DLL_API(av_init_packet)
	, XAMP_LOAD_DLL_API(av_packet_unref)
	, XAMP_LOAD_DLL_API(avcodec_send_packet)
	, XAMP_LOAD_DLL_API(avcodec_send_frame)
	, XAMP_LOAD_DLL_API(avcodec_receive_frame)
	, XAMP_LOAD_DLL_API(avcodec_receive_packet)
	, XAMP_LOAD_DLL_API(avcodec_flush_buffers)
	, XAMP_LOAD_DLL_API(av_get_bits_per_sample)
	, XAMP_LOAD_DLL_API(avcodec_find_decoder_by_name)
	, XAMP_LOAD_DLL_API(avcodec_find_encoder)
	, XAMP_LOAD_DLL_API(avcodec_configuration)
	, XAMP_LOAD_DLL_API(avcodec_parameters_from_context)
	, XAMP_LOAD_DLL_API(av_codec_iterate)
	, XAMP_LOAD_DLL_API(av_packet_rescale_ts)
	, XAMP_LOAD_DLL_API(av_packet_free)
    , XAMP_LOAD_DLL_API(avcodec_free_context)
    , XAMP_LOAD_DLL_API(avcodec_parameters_to_context)
	, XAMP_LOAD_DLL_API(avcodec_find_encoder_by_name) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

AvUtilLib::AvUtilLib() try
    : module_(OpenSharedLibrary("avutil-57"))
	, XAMP_LOAD_DLL_API(av_free)
	, XAMP_LOAD_DLL_API(av_frame_free)
	, XAMP_LOAD_DLL_API(av_frame_unref)
	, XAMP_LOAD_DLL_API(av_frame_get_buffer)
	, XAMP_LOAD_DLL_API(av_get_bytes_per_sample)
	, XAMP_LOAD_DLL_API(av_strerror)
	, XAMP_LOAD_DLL_API(av_frame_alloc)
	, XAMP_LOAD_DLL_API(av_frame_make_writable)
	, XAMP_LOAD_DLL_API(av_malloc)
	, XAMP_LOAD_DLL_API(av_samples_get_buffer_size)
	, XAMP_LOAD_DLL_API(av_log_set_callback)
	, XAMP_LOAD_DLL_API(av_log_format_line)
	, XAMP_LOAD_DLL_API(av_log_set_level)
	, XAMP_LOAD_DLL_API(av_dict_set)
	, XAMP_LOAD_DLL_API(av_get_channel_layout_nb_channels)
	, XAMP_LOAD_DLL_API(av_audio_fifo_alloc) 
	, XAMP_LOAD_DLL_API(av_audio_fifo_size)
	, XAMP_LOAD_DLL_API(av_audio_fifo_realloc)
	, XAMP_LOAD_DLL_API(av_audio_fifo_write) 
	, XAMP_LOAD_DLL_API(av_audio_fifo_free)
	, XAMP_LOAD_DLL_API(av_samples_fill_arrays)
	, XAMP_LOAD_DLL_API(av_rescale_q)
	, XAMP_LOAD_DLL_API(av_sample_fmt_is_planar)
	, XAMP_LOAD_DLL_API(av_dict_get)
	, XAMP_LOAD_DLL_API(av_opt_set)
	, XAMP_LOAD_DLL_API(av_dict_free)
	, XAMP_LOAD_DLL_API(av_freep)
	, XAMP_LOAD_DLL_API(av_samples_alloc)
	, XAMP_LOAD_DLL_API(av_opt_set_int)
	, XAMP_LOAD_DLL_API(av_opt_set_sample_fmt)
	, XAMP_LOAD_DLL_API(av_get_sample_fmt_name) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

AvSwLib::AvSwLib() try
    : module_(OpenSharedLibrary("swresample-4"))
	, XAMP_LOAD_DLL_API(swr_free)
	, XAMP_LOAD_DLL_API(swr_alloc_set_opts)
	, XAMP_LOAD_DLL_API(swr_convert)
	, XAMP_LOAD_DLL_API(swr_init)
	, XAMP_LOAD_DLL_API(swr_close)
	, XAMP_LOAD_DLL_API(swr_get_out_samples)
	, XAMP_LOAD_DLL_API(swr_alloc) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

static void LogPrintf(void* ptr, int level, const char* fmt, va_list vl) {
	va_list valist;
	char message[1024]{};

	va_copy(valist, vl);
	int print_prefix = 1;
	LIBAV_LIB.Util->av_log_format_line(ptr, level, fmt, valist, message, sizeof(message), &print_prefix);
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
	Free();
}

void AvLib::Free() {
	if (Format != nullptr) {
		Format->avformat_network_deinit();
	}

	Util.reset();
	Swr.reset();
	Codec.reset();
	Format.reset();
}

AvLib::AvLib() {
	logger = XampLoggerFactory.GetLogger(kLibAvLoggerName);
	XAMP_LOG_D(logger, "Load {} success.", LIBAVCODEC_IDENT);

	Format = MakeAlign<AvFormatLib>();
	Codec = MakeAlign<AvCodecLib>();
	Swr = MakeAlign<AvSwLib>();
	Util = MakeAlign<AvUtilLib>();

	Util->av_log_set_callback(LogPrintf);
	Util->av_log_set_level(AV_LOG_FATAL);

	Format->avformat_network_init();

	const auto level = logger->GetLevel();
	logger->SetLevel(LOG_LEVEL_DEBUG);
	XAMP_LOG_D(logger, Codec->avcodec_configuration());
	logger->SetLevel(level);

	XAMP_LOG_D(logger, "Network init.");
}

HashSet<std::string> AvLib::GetSupportFileExtensions() const {
	HashSet<std::string> result;
	HashSet<std::string> ordered_extension;

	//const auto level = logger->GetLevel();
	//logger->SetLevel(LOG_LEVEL_DEBUG);

    const AVInputFormat *output_format;
    void *opaque = nullptr;
    while ((output_format = Format->av_demuxer_iterate(&opaque)) != nullptr){
        if (output_format->extensions) {
            auto result = String::Split(output_format->extensions, ",");
            if (!result.empty()) {
                for (const auto& extension : result) {
                    auto ext = String::AsStdString(extension);
                    String::LTrim(ext);
                    String::RTrim(ext);
                    if (ordered_extension.find(ext) == ordered_extension.end()) {
                        ordered_extension.insert(ext);
                    }
                }
            } else {
                ordered_extension.insert(String::Format(".{}", output_format->extensions));
            }
        }
    }

	// Workaround!
	ordered_extension.insert("wav");
	ordered_extension.insert("mp3");
	ordered_extension.insert("dsf");
	ordered_extension.insert("dff");
	ordered_extension.erase("m3u8");

	result.reserve(ordered_extension.size());

	for (const auto& extension : ordered_extension) {
		const auto file_extensions = String::Format(".{}", extension);
		XAMP_LOG_T(logger, "Load Libav format extensions: {}", file_extensions);
		result.insert(file_extensions);
	}

	//logger->SetLevel(level);
	return result;
}

XAMP_STREAM_NAMESPACE_END
