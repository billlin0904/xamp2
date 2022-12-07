//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/file.h>
#include <libswresample/swresample.h>
#include <libavutil/rational.h>
#include <libavutil/opt.h>
}

#include <base/base.h>
#include <base/align_ptr.h>
#include <base/dll.h>
#include <base/singleton.h>
#include <base/assert.h>
#include <stream/stream.h>

namespace xamp::stream {

class AvFormatLib final {
public:
    AvFormatLib();

private:
    ModuleHandle module_;

public:
   XAMP_DECLARE_DLL_NAME(avformat_open_input);
   XAMP_DECLARE_DLL_NAME(avformat_close_input);
   XAMP_DECLARE_DLL_NAME(avformat_find_stream_info);
   XAMP_DECLARE_DLL_NAME(av_seek_frame);
   XAMP_DECLARE_DLL_NAME(av_read_frame);
};

class AvCodecLib final {
public:
    AvCodecLib();

private:
    ModuleHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(avcodec_close);
    XAMP_DECLARE_DLL_NAME(avcodec_open2);
    XAMP_DECLARE_DLL_NAME(avcodec_find_decoder);
    XAMP_DECLARE_DLL_NAME(av_packet_alloc);
    XAMP_DECLARE_DLL_NAME(av_init_packet);
    XAMP_DECLARE_DLL_NAME(av_packet_unref);
    XAMP_DECLARE_DLL_NAME(avcodec_send_packet);
    XAMP_DECLARE_DLL_NAME(avcodec_receive_frame);
    XAMP_DECLARE_DLL_NAME(avcodec_flush_buffers);
    XAMP_DECLARE_DLL_NAME(av_get_bits_per_sample);
};

class AvUtilLib final {
public:
    AvUtilLib();

private:
    ModuleHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(av_free);
    XAMP_DECLARE_DLL_NAME(av_frame_unref);
    XAMP_DECLARE_DLL_NAME(av_get_bytes_per_sample);
    XAMP_DECLARE_DLL_NAME(av_strerror);
    XAMP_DECLARE_DLL_NAME(av_frame_alloc);
    XAMP_DECLARE_DLL_NAME(av_malloc);
    XAMP_DECLARE_DLL_NAME(av_samples_get_buffer_size);
    XAMP_DECLARE_DLL_NAME(av_log_set_callback);
    XAMP_DECLARE_DLL_NAME(av_log_format_line);
    XAMP_DECLARE_DLL_NAME(av_log_set_level);
};

class AvSwLib final {
public:
    AvSwLib();

private:
    ModuleHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(swr_free);
    XAMP_DECLARE_DLL_NAME(swr_alloc_set_opts);
    XAMP_DECLARE_DLL_NAME(swr_convert);
    XAMP_DECLARE_DLL_NAME(swr_init);
    XAMP_DECLARE_DLL_NAME(swr_close);
};

class AvLib {
public:
    AvLib();

    AlignPtr<AvFormatLib> FormatLib;
    AlignPtr<AvCodecLib> CodecLib;
    AlignPtr<AvSwLib> SwrLib;
    AlignPtr<AvUtilLib> UtilLib;
};

#define LIBAV_LIB Singleton<AvLib>::GetInstance()

template <typename T>
struct AvResourceDeleter;

template <>
struct AvResourceDeleter<AVFormatContext> {
    void operator()(AVFormatContext* p) const {
        LIBAV_LIB.FormatLib->avformat_close_input(&p);
    }
};

template <>
struct AvResourceDeleter<AVCodecContext> {
    void operator()(AVCodecContext* p) const {
        LIBAV_LIB.CodecLib->avcodec_close(p);
    }
};

template <>
struct AvResourceDeleter<SwrContext> {
    void operator()(SwrContext* p) const {
        LIBAV_LIB.SwrLib->swr_free(&p);
    }
};

template <>
struct AvResourceDeleter<AVPacket> {
    void operator()(AVPacket* p) const {
        XAMP_ASSERT(p != nullptr);
        LIBAV_LIB.UtilLib->av_free(p);
    }
};

template <>
struct AvResourceDeleter<AVFrame> {
    void operator()(AVFrame* p) const {
        XAMP_ASSERT(p != nullptr);
        LIBAV_LIB.UtilLib->av_free(p);
    }
};

template <typename T>
using AvPtr = std::unique_ptr<T, AvResourceDeleter<T>>;

}
