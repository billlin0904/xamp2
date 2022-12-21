//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>


#include <stream/stream.h>
#include <r8bsrc.h>
#include <base/dll.h>

namespace xamp::stream {

class R8brainLib {
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

}
