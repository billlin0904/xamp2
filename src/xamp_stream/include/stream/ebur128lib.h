//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/dll.h>
#include <stream/stream.h>

XAMP_STREAM_NAMESPACE_BEGIN

class Ebur128Lib final {
public:
	Ebur128Lib();

private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(ebur128_init);
	XAMP_DECLARE_DLL_NAME(ebur128_destroy);
	XAMP_DECLARE_DLL_NAME(ebur128_set_channel);
	XAMP_DECLARE_DLL_NAME(ebur128_add_frames_float);
	XAMP_DECLARE_DLL_NAME(ebur128_true_peak);
	XAMP_DECLARE_DLL_NAME(ebur128_sample_peak);
	XAMP_DECLARE_DLL_NAME(ebur128_loudness_global);
	XAMP_DECLARE_DLL_NAME(ebur128_loudness_global_multiple);
};

Ebur128Lib::Ebur128Lib()
#ifdef XAMP_OS_WIN
	: module_(LoadSharedLibrary("ebur128.dll"))
#else
	: module_(LoadSharedLibrary("libebur128.dylib"))
#endif
	, XAMP_LOAD_DLL_API(ebur128_init)
	, XAMP_LOAD_DLL_API(ebur128_destroy)
	, XAMP_LOAD_DLL_API(ebur128_set_channel)
	, XAMP_LOAD_DLL_API(ebur128_add_frames_float)
	, XAMP_LOAD_DLL_API(ebur128_true_peak)
	, XAMP_LOAD_DLL_API(ebur128_sample_peak)
	, XAMP_LOAD_DLL_API(ebur128_loudness_global)
	, XAMP_LOAD_DLL_API(ebur128_loudness_global_multiple) {
}

#define EBUR128_LIB SharedSingleton<Ebur128Lib>::GetInstance()


XAMP_STREAM_NAMESPACE_END

