//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <base/dll.h>
#include <ebur128/ebur128.h>

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
	XAMP_DECLARE_DLL_NAME(ebur128_prev_true_peak);
	XAMP_DECLARE_DLL_NAME(ebur128_prev_sample_peak);
	XAMP_DECLARE_DLL_NAME(ebur128_get_version);
};

inline Ebur128Lib::Ebur128Lib()
	: module_(OpenSharedLibrary("ebur128"))
	, XAMP_LOAD_DLL_API(ebur128_init)
	, XAMP_LOAD_DLL_API(ebur128_destroy)
	, XAMP_LOAD_DLL_API(ebur128_set_channel)
	, XAMP_LOAD_DLL_API(ebur128_add_frames_float)
	, XAMP_LOAD_DLL_API(ebur128_true_peak)
	, XAMP_LOAD_DLL_API(ebur128_sample_peak)
	, XAMP_LOAD_DLL_API(ebur128_loudness_global)
	, XAMP_LOAD_DLL_API(ebur128_loudness_global_multiple)
	, XAMP_LOAD_DLL_API(ebur128_prev_true_peak)
	, XAMP_LOAD_DLL_API(ebur128_prev_sample_peak)
	, XAMP_LOAD_DLL_API(ebur128_get_version) {
}

XAMP_STREAM_NAMESPACE_END