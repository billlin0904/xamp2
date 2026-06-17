//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/logger.h>
#include <base/base.h>
#include <base/memory.h>
#include <base/memory.h>
#include <base/shared_singleton.h>

XAMP_BASE_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(CrashHandler);

class XAMP_BASE_API CrashHandler {
public:
	XAMP_DECLARE_SINGLETON_NAME()

	CrashHandler();

	XAMP_PIMPL(CrashHandler)

	void setProcessExceptionHandlers();

	void setThreadExceptionHandlers();

	static void dumpStackInfo(void* info);

	void cleanup();
private:
	class CrashHandlerImpl;
	ScopedPtr<CrashHandlerImpl> impl_;
};

#define XampCrashHandler SharedSingleton<CrashHandler>::getInstance()

XAMP_BASE_NAMESPACE_END
