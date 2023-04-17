//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/align_ptr.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API CrashHandler {
public:
	CrashHandler();

	XAMP_PIMPL(CrashHandler)

	void SetProcessExceptionHandlers();

	void SetThreadExceptionHandlers();
private:
	class CrashHandlerImpl;
	AlignPtr<CrashHandlerImpl> impl_;
};

XAMP_BASE_NAMESPACE_END
