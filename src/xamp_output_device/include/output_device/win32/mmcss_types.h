//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/enum.h>

namespace xamp::output_device::win32 {

inline constexpr std::wstring_view MMCSS_PROFILE_AUDIO(L"Audio");
inline constexpr std::wstring_view MMCSS_PROFILE_CAPTURE(L"Capture");
inline constexpr std::wstring_view MMCSS_MODE_DISTRIBUTION(L"Distribution");
inline constexpr std::wstring_view MMCSS_PROFILE_GAME(L"Games");
inline constexpr std::wstring_view MMCSS_PROFILE_PLAYBACK(L"Playback");
inline constexpr std::wstring_view MMCSS_PROFILE_PRO_AUDIO(L"Pro Audio");
inline constexpr std::wstring_view MMCSS_PROFILE_WINDOWS_MANAGER(L"Window Manager");

MAKE_XAMP_ENUM(MmcssThreadPriority,
	      MMCSS_THREAD_PRIORITY_NORMAL,
	      MMCSS_THREAD_PRIORITY_HIGH,
	      MMCSS_THREAD_PRIORITY_CRITICAL)

}
#endif
