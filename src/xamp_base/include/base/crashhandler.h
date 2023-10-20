//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/logger.h>
#include <base/base.h>
#include <base/pimplptr.h>
#include <base/align_ptr.h>
#include <base/shared_singleton.h>

XAMP_BASE_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(CrashHandler);

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

#define SharedCrashHandler SharedSingleton<CrashHandler>::GetInstance()

XAMP_BASE_NAMESPACE_END
