//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <stream/stream.h>
#include <r8bsrc.h>
#include <base/dll.h>

namespace xamp::stream {

class R8brainLib {
public:
	R8brainLib();

	XAMP_DISABLE_COPY(R8brainLib)

private:
	ModuleHandle module_;

public:
	XAMP_DECLARE_DLL(r8b_create) r8b_create;
	XAMP_DECLARE_DLL(r8b_delete) r8b_delete;
	XAMP_DECLARE_DLL(r8b_clear) r8b_clear;
	XAMP_DECLARE_DLL(r8b_process) r8b_process;
};

}

#endif