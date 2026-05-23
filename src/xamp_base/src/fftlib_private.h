//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#if defined(XAMP_OS_WIN)

#include <base/dll.h>
#include <base/shared_singleton.h>

#define MKL_DIRECT_CALL
#include <mkl_service.h>
#include <mkl_dfti.h>
#include <mkl.h>

XAMP_BASE_NAMESPACE_BEGIN

class MKLLib final {
public:
	XAMP_DECLARE_SINGLETON_NAME()

	MKLLib();

	XAMP_DISABLE_COPY(MKLLib)
private:
	SharedLibraryHandle mkl_core_;
	SharedLibraryHandle libiomp_;
	SharedLibraryHandle mkl_intel_thread_;
	SharedLibraryHandle mkl_cdft_core_;
	SharedLibraryHandle mkl_def_;
	SharedLibraryHandle mkl_mc3_;
	SharedLibraryHandle mkl_avx2_;
	SharedLibraryHandle mkl_avx512_;
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL(MKL_malloc) MKL_malloc;
	XAMP_DECLARE_DLL(MKL_free) MKL_free;
	XAMP_DECLARE_DLL(DftiErrorClass) DftiErrorClass;
	XAMP_DECLARE_DLL(DftiFreeDescriptor) DftiFreeDescriptor;
	XAMP_DECLARE_DLL(DftiCreateDescriptor_s_1d) DftiCreateDescriptor_s_1d;
	XAMP_DECLARE_DLL(DftiCreateDescriptor_s_md) DftiCreateDescriptor_s_md;
	XAMP_DECLARE_DLL(DftiCreateDescriptor_d_1d) DftiCreateDescriptor_d_1d;
	XAMP_DECLARE_DLL(DftiCreateDescriptor_d_md) DftiCreateDescriptor_d_md;
	XAMP_DECLARE_DLL(DftiComputeForward) DftiComputeForward;
	XAMP_DECLARE_DLL(DftiCreateDescriptor) DftiCreateDescriptor_;
	XAMP_DECLARE_DLL(DftiSetValue) DftiSetValue;
	XAMP_DECLARE_DLL(DftiCommitDescriptor) DftiCommitDescriptor;
	XAMP_DECLARE_DLL(DftiComputeBackward) DftiComputeBackward;
	XAMP_DECLARE_DLL(DftiErrorMessage) DftiErrorMessage;
};

#define MklDLL SharedSingleton<MKLLib>::GetInstance()

XAMP_BASE_NAMESPACE_END

#endif
