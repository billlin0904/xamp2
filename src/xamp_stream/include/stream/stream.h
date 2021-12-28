//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN
#ifdef STREAM_API_EXPORTS
    #define XAMP_STREAM_API __declspec(dllexport)
#else
    #define XAMP_STREAM_API __declspec(dllimport)
#endif
#else
#define XAMP_STREAM_API
#endif

namespace xamp::stream {
	using namespace base;

	struct CompressorParameters;
	struct EQSettings;

    class ISampleRateConverter;
	class IAudioProcessor;
	class IAudioStream;
	class ICDDevice;
	class IFileEncoder;
	class FileStream;
	class IDsdStream;
}
