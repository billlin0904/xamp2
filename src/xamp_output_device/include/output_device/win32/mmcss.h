//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/output_device.h>
#include <output_device/win32/mmcss_types.h>
#include <base/memory.h>
#include <base/pimplptr.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

/*
* Mmcss 
* https://docs.microsoft.com/en-us/windows/win32/coreaudio/using-mmcss
* 
* Mmcss is a Windows API that allows applications to boost the priority 
* of threads that perform time-critical tasks.
*/
class Mmcss final {
public:
	/*
	* Constructor.
	*/
	Mmcss();

	XAMP_PIMPL(Mmcss)

	/*
	* Boost current thread priority.
	* 
	* @param[in] task_name
	* @param[in] priority
	*/
	void BoostPriority(std::wstring_view task_name = kMmcssProfileProAudio,
		MmcssThreadPriority priority = MmcssThreadPriority::MMCSS_THREAD_PRIORITY_NORMAL) noexcept;

	/*
	* Revert current thread priority.
	*/
	void RevertPriority() noexcept;
private:
	class MmcssImpl;
	ScopedPtr<MmcssImpl> impl_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN

