//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>

#include <stream/stream.h>
#include <DLL/r8bsrc.h>

XAMP_STREAM_NAMESPACE_BEGIN

class R8brainLib final {
public:
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

XAMP_STREAM_NAMESPACE_END
