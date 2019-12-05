//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <string_view>
#include <output_device/output_device.h>

#ifdef _WIN32
namespace xamp::output_device::win32 {

using namespace base;

constexpr std::wstring_view MMCSS_PROFILE_AUDIO(L"Audio");
constexpr std::wstring_view MMCSS_PROFILE_CAPTURE(L"Capture");
constexpr std::wstring_view MMCSS_MODE_DISTRIBUTION(L"Distribution");
constexpr std::wstring_view MMCSS_PROFILE_GAME(L"Games");
constexpr std::wstring_view MMCSS_PROFILE_PLAYBACK(L"Playback");
constexpr std::wstring_view MMCSS_PROFILE_PRO_AUDIO(L"Pro Audio");
constexpr std::wstring_view MMCSS_PROFILE_WINDOWS_MANAGER(L"Window Manager");

enum class MmcssThreadPriority {
	MMCSS_THREAD_PRIORITY_NORMAL,
	MMCSS_THREAD_PRIORITY_HIGH,
	MMCSS_THREAD_PRIORITY_CRITICAL
};

}
#endif
