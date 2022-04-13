//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/fastmutex.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#endif

namespace xamp::base {

class XAMP_BASE_API CrashHandler {
public:
	CrashHandler();

	~CrashHandler();

	XAMP_DISABLE_COPY(CrashHandler)

	void SetProcessExceptionHandlers();

	void SetThreadExceptionHandlers();
private:
	static void Dump(void *info);
#ifdef XAMP_OS_WIN
	static int __cdecl NewHandler(size_t);

	static void GetExceptionPointers(DWORD exception_code, EXCEPTION_POINTERS* exception_pointers);

	static void CreateMiniDump(EXCEPTION_POINTERS* exception_pointers);

	static LONG WINAPI SehHandler(PEXCEPTION_POINTERS exception_pointers);

	static LONG WINAPI VectoredHandler(PEXCEPTION_POINTERS exception_pointers);

	static void __cdecl TerminateHandler();

	static void __cdecl UnexpectedHandler();

	static void __cdecl PureCallHandler();

	static void __cdecl InvalidParameterHandler(const wchar_t* expression,
		const wchar_t* function,
		const wchar_t* file,
		unsigned int line,
		uintptr_t reserved);

	static void StackDump();
#else
	void InstallSignalHandler();
	static void CrashSignalHandler(int signal_number, siginfo_t* info, void*);
#endif
	static FastMutex mutex_;
};

}
