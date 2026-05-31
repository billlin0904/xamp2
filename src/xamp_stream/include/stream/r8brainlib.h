//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/dll.h>
#include <base/logger.h>

#include <stream/stream.h>

#ifndef XAMP_OS_WIN
#define R8BSRC_DECL
#endif
#include <DLL/r8bsrc.h>

XAMP_STREAM_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN
class R8brainLib final {
public:
	XAMP_DECLARE_SINGLETON_NAME()

	R8brainLib();

	XAMP_DISABLE_COPY(R8brainLib)
private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(r8b_create);
	XAMP_DECLARE_DLL_NAME(r8b_delete);
	XAMP_DECLARE_DLL_NAME(r8b_clear);
	XAMP_DECLARE_DLL_NAME(r8b_process);
};

inline R8brainLib::R8brainLib() try
	: module_(OpenSharedLibrary("r8bsrc"))
	, XAMP_LOAD_DLL_API(r8b_create)
	, XAMP_LOAD_DLL_API(r8b_delete)
	, XAMP_LOAD_DLL_API(r8b_clear)
	, XAMP_LOAD_DLL_API(r8b_process) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}
#else
class R8brainLib final {
public:
	XAMP_DECLARE_SINGLETON_NAME()

	CR8BResampler r8b_create(
		const double input_sample_rate,
		const double output_sample_rate,
		const int max_input_length,
		const double required_transition_band,
		const ER8BResamplerRes resolution) const {
		return ::r8b_create(input_sample_rate,
			output_sample_rate,
			max_input_length,
			required_transition_band,
			resolution);
	}

	void r8b_delete(CR8BResampler const resampler) const {
		::r8b_delete(resampler);
	}

	void r8b_clear(CR8BResampler const resampler) const {
		::r8b_clear(resampler);
	}

	int r8b_process(CR8BResampler const resampler, double* const input, const int length, double*& output) const {
		return ::r8b_process(resampler, input, length, output);
	}
};
#endif

#define LibR8brainDLL SharedSingleton<R8brainLib>::GetInstance()

XAMP_STREAM_NAMESPACE_END
