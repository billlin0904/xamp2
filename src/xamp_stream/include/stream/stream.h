//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#include <complex>
#include <valarray>

#ifdef XAMP_OS_WIN
#ifdef STREAM_API_EXPORTS
    #define XAMP_STREAM_API __declspec(dllexport)
#else
    #define XAMP_STREAM_API __declspec(dllimport)
#endif
#elif defined(XAMP_OS_MAC)
#define XAMP_STREAM_API __attribute__((visibility("default")))
#endif

namespace xamp::stream {
	using namespace base;

	struct CompressorParameters;
	struct EqSettings;

	class STFT;
	class MBDiscId;

    class ISampleWriter;
	class IAudioProcessor;
	class IAudioStream;
	class ICDDevice;
	class IFileEncoder;
	class FileStream;
	class IDsdStream;
	class IDSPManager;
	class Pcm2DsdSampleWriter;

	using Complex = std::complex<float>;
	using ComplexValarray = std::valarray<Complex>;
}
