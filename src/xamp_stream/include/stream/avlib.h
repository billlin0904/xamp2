//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <stream/stream.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/file.h>
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>
#include <libavutil/rational.h>
#include <libavutil/opt.h>
}

#include <base/base.h>
#include <base/exception.h>
#include <base/logger.h>
#include <base/align_ptr.h>
#include <base/dll.h>
#include <base/singleton.h>
#include <base/assert.h>
#include <base/stl.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API AvException final : public Exception {
public:
    explicit AvException(int32_t error_code);

    ~AvException() override;

	[[nodiscard]] char const* what() const noexcept override;

	[[nodiscard]] int32_t GetErrorCode() const noexcept;

    private:
	int32_t error_code_;
	std::string error_msg_;
};

#define AvIfFailedThrow(expr) \
	do { \
		auto error = (expr); \
		if (error != 0) { \
			throw AvException(error); \
		} \
	} while (false)

class AvFormatLib final {
public:
    AvFormatLib();

    XAMP_DISABLE_COPY(AvFormatLib)

private:
    SharedLibraryHandle module_;

public:
   XAMP_DECLARE_DLL_NAME(avformat_open_input);
   XAMP_DECLARE_DLL_NAME(avformat_close_input);
   XAMP_DECLARE_DLL_NAME(avformat_find_stream_info);
   XAMP_DECLARE_DLL_NAME(av_seek_frame);
   XAMP_DECLARE_DLL_NAME(av_read_frame);
   XAMP_DECLARE_DLL_NAME(av_write_frame);
   XAMP_DECLARE_DLL_NAME(avformat_write_header);
   XAMP_DECLARE_DLL_NAME(avformat_network_init);
   XAMP_DECLARE_DLL_NAME(avformat_network_deinit);
   XAMP_DECLARE_DLL_NAME(avformat_alloc_context);
   XAMP_DECLARE_DLL_NAME(avformat_new_stream);
   XAMP_DECLARE_DLL_NAME(avformat_query_codec);
   XAMP_DECLARE_DLL_NAME(av_oformat_next);
   XAMP_DECLARE_DLL_NAME(avformat_alloc_output_context2);
   XAMP_DECLARE_DLL_NAME(avio_open);
   XAMP_DECLARE_DLL_NAME(av_interleaved_write_frame);
};

class AvCodecLib final {
public:
    AvCodecLib();

    XAMP_DISABLE_COPY(AvCodecLib)

private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(avcodec_close);
    XAMP_DECLARE_DLL_NAME(avcodec_open2);
    XAMP_DECLARE_DLL_NAME(avcodec_alloc_context3);
    XAMP_DECLARE_DLL_NAME(avcodec_find_decoder);
    XAMP_DECLARE_DLL_NAME(av_packet_alloc);
    XAMP_DECLARE_DLL_NAME(av_init_packet);
    XAMP_DECLARE_DLL_NAME(av_packet_unref);
    XAMP_DECLARE_DLL_NAME(avcodec_send_packet);
    XAMP_DECLARE_DLL_NAME(avcodec_send_frame);
    XAMP_DECLARE_DLL_NAME(avcodec_receive_frame);
    XAMP_DECLARE_DLL_NAME(avcodec_receive_packet);
    XAMP_DECLARE_DLL_NAME(avcodec_flush_buffers);
    XAMP_DECLARE_DLL_NAME(av_get_bits_per_sample);
    XAMP_DECLARE_DLL_NAME(avcodec_find_decoder_by_name);
    XAMP_DECLARE_DLL_NAME(avcodec_find_encoder);
    XAMP_DECLARE_DLL_NAME(avcodec_configuration);
    XAMP_DECLARE_DLL_NAME(avcodec_parameters_from_context);
    XAMP_DECLARE_DLL_NAME(av_codec_next);
    XAMP_DECLARE_DLL_NAME(av_packet_rescale_ts);
};

class AvUtilLib final {
public:
    AvUtilLib();

    XAMP_DISABLE_COPY(AvUtilLib)
private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(av_free);
    XAMP_DECLARE_DLL_NAME(av_frame_unref);
    XAMP_DECLARE_DLL_NAME(av_frame_get_buffer);
    XAMP_DECLARE_DLL_NAME(av_get_bytes_per_sample);
    XAMP_DECLARE_DLL_NAME(av_strerror);
    XAMP_DECLARE_DLL_NAME(av_frame_alloc);
    XAMP_DECLARE_DLL_NAME(av_frame_make_writable);
    XAMP_DECLARE_DLL_NAME(av_malloc);
    XAMP_DECLARE_DLL_NAME(av_samples_get_buffer_size);
    XAMP_DECLARE_DLL_NAME(av_log_set_callback);
    XAMP_DECLARE_DLL_NAME(av_log_format_line);
    XAMP_DECLARE_DLL_NAME(av_log_set_level);
    XAMP_DECLARE_DLL_NAME(av_dict_set);
    XAMP_DECLARE_DLL_NAME(av_get_channel_layout_nb_channels);
    XAMP_DECLARE_DLL_NAME(av_audio_fifo_alloc);
    XAMP_DECLARE_DLL_NAME(av_audio_fifo_size);
    XAMP_DECLARE_DLL_NAME(av_audio_fifo_realloc);
    XAMP_DECLARE_DLL_NAME(av_audio_fifo_write);
    XAMP_DECLARE_DLL_NAME(av_audio_fifo_free);
    XAMP_DECLARE_DLL_NAME(av_samples_fill_arrays);
    XAMP_DECLARE_DLL_NAME(av_rescale_q);
    XAMP_DECLARE_DLL_NAME(av_sample_fmt_is_planar);
    XAMP_DECLARE_DLL_NAME(av_dict_get);
};

class AvSwLib final {
public:
    AvSwLib();

    XAMP_DISABLE_COPY(AvSwLib)
private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(swr_free);
    XAMP_DECLARE_DLL_NAME(swr_alloc_set_opts);
    XAMP_DECLARE_DLL_NAME(swr_convert);
    XAMP_DECLARE_DLL_NAME(swr_init);
    XAMP_DECLARE_DLL_NAME(swr_close);
};

class AvLib final {
public:
    AvLib();

    ~AvLib();

    XAMP_DISABLE_COPY(AvLib)

    HashSet<std::string> GetSupportFileExtensions() const;

    AlignPtr<AvFormatLib> FormatLib;
    AlignPtr<AvCodecLib> CodecLib;
    AlignPtr<AvSwLib> SwrLib;
    AlignPtr<AvUtilLib> UtilLib;

    LoggerPtr logger;
};

#define LIBAV_LIB Singleton<AvLib>::GetInstance()

template <typename T>
struct AvResourceDeleter;

template <>
struct AvResourceDeleter<AVFormatContext> {
    void operator()(AVFormatContext* p) const {
        XAMP_EXPECTS(p != nullptr);
        LIBAV_LIB.FormatLib->avformat_close_input(&p);
    }
};

template <>
struct AvResourceDeleter<AVCodecContext> {
    void operator()(AVCodecContext* p) const {
        XAMP_EXPECTS(p != nullptr);
        LIBAV_LIB.CodecLib->avcodec_close(p);
    }
};

template <>
struct AvResourceDeleter<SwrContext> {
    void operator()(SwrContext* p) const {
        XAMP_EXPECTS(p != nullptr);
        LIBAV_LIB.SwrLib->swr_free(&p);
    }
};

template <>
struct AvResourceDeleter<AVPacket> {
    void operator()(AVPacket* p) const {
        XAMP_EXPECTS(p != nullptr);
        LIBAV_LIB.UtilLib->av_free(p);
    }
};

template <>
struct AvResourceDeleter<AVFrame> {
    void operator()(AVFrame* p) const {
        XAMP_EXPECTS(p != nullptr);
        LIBAV_LIB.UtilLib->av_free(p);
    }
};

template <>
struct AvResourceDeleter<AVAudioFifo> {
    void operator()(AVAudioFifo* p) const {
        XAMP_EXPECTS(p != nullptr);
        LIBAV_LIB.UtilLib->av_audio_fifo_free(p);
    }
};

template <typename T>
using AvPtr = std::unique_ptr<T, AvResourceDeleter<T>>;

XAMP_STREAM_NAMESPACE_END