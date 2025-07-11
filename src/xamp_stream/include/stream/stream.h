//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#include <complex>
#include <valarray>
#include <vector>

#ifdef XAMP_OS_WIN
#ifdef STREAM_API_EXPORTS
    #define XAMP_STREAM_API __declspec(dllexport)
#else
    #define XAMP_STREAM_API __declspec(dllimport)
#endif
#elif defined(XAMP_OS_MAC)
#define XAMP_STREAM_API __attribute__((visibility("default")))
#endif

#define XAMP_STREAM_NAMESPACE_BEGIN namespace xamp { namespace stream {
#define XAMP_STREAM_NAMESPACE_END } }

#define XAMP_STREAM_UTIL_NAMESPACE_BEGIN namespace xamp { namespace stream { namespace bass_util {
#define XAMP_STREAM_UTIL_NAMESPACE_END } } }

XAMP_STREAM_NAMESPACE_BEGIN
	using namespace base;

    struct CompressorConfig;
    struct EqSettings;

    class STFT;
    class MBDiscId;

    class ISampleWriter;
    class IAudioProcessor;
    class IAudioStream;
    class ICDDevice;
    class IFileEncoder;
    class IFile;
    class FileStream;
    class IDsdStream;
    class ArchiveStream;

    class IDSPManager;

    using Complex = std::complex<float>;
    using ComplexValarray = std::vector<Complex>;
XAMP_STREAM_NAMESPACE_END
