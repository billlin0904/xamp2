//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <string_view>

#include <base/align_ptr.h>
#include <output_device/output_device.h>

#ifdef _WIN32
namespace xamp::output_device::win32 {

using namespace base;

extern XAMP_OUTPUT_DEVICE_API const std::wstring_view MMCSS_PROFILE_AUDIO;
extern XAMP_OUTPUT_DEVICE_API const std::wstring_view MMCSS_PROFILE_CAPTURE;
extern XAMP_OUTPUT_DEVICE_API const std::wstring_view MMCSS_MODE_DISTRIBUTION;
extern XAMP_OUTPUT_DEVICE_API const std::wstring_view MMCSS_PROFILE_GAME;
extern XAMP_OUTPUT_DEVICE_API const std::wstring_view MMCSS_PROFILE_PLAYBACK;
extern XAMP_OUTPUT_DEVICE_API const std::wstring_view MMCSS_PROFILE_PRO_AUDIO;
extern XAMP_OUTPUT_DEVICE_API const std::wstring_view MMCSS_PROFILE_WINDOWS_MANAGER;

enum class MmcssThreadPriority {
	MMCSS_THREAD_PRIORITY_NORMAL,
	MMCSS_THREAD_PRIORITY_HIGH,
	MMCSS_THREAD_PRIORITY_CRITICAL
};

class Mmcss {
public:
	static Mmcss& Instance() {
		static Mmcss instance;
		return instance;
	}

	XAMP_DISABLE_COPY(Mmcss)

	void BoostPriority();

	void RevertPriority();
private:
	Mmcss();

	~Mmcss();

	class MmcssImpl;
	AlignPtr<MmcssImpl> impl_;
};

}
#endif
