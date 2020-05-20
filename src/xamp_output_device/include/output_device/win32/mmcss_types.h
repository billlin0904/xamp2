//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <string_view>
#include <base/enum.h>
#include <output_device/output_device.h>

#ifdef XAMP_OS_WIN
namespace xamp::output_device::win32 {

using namespace base;

static constexpr std::wstring_view MMCSS_PROFILE_AUDIO(L"Audio");
static constexpr std::wstring_view MMCSS_PROFILE_CAPTURE(L"Capture");
static constexpr std::wstring_view MMCSS_MODE_DISTRIBUTION(L"Distribution");
static constexpr std::wstring_view MMCSS_PROFILE_GAME(L"Games");
static constexpr std::wstring_view MMCSS_PROFILE_PLAYBACK(L"Playback");
static constexpr std::wstring_view MMCSS_PROFILE_PRO_AUDIO(L"Pro Audio");
static constexpr std::wstring_view MMCSS_PROFILE_WINDOWS_MANAGER(L"Window Manager");

MAKE_ENUM(MmcssThreadPriority,
	      MMCSS_THREAD_PRIORITY_NORMAL,
	      MMCSS_THREAD_PRIORITY_HIGH,
	      MMCSS_THREAD_PRIORITY_CRITICAL)

}
#endif
