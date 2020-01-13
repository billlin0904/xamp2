//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#ifdef _WIN32
#ifdef STREAM_API_EXPORTS
    #define XAMP_STREAM_API __declspec(dllexport)
#else
    #define XAMP_STREAM_API __declspec(dllimport)
#endif
#define ENABLE_FFMPEG 1
#else
#define XAMP_STREAM_API
#endif

namespace xamp::stream {
	using namespace base;

	class AudioStream;
	class FileStream;
	class DSDStream;
	class AvFileStream;
	class BassFileStream;
}
