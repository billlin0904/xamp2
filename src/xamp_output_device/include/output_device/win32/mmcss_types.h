//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/enum.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

inline constexpr std::wstring_view kMmcssProfileAudio(L"Audio");
inline constexpr std::wstring_view kMmcssProfileCapture(L"Capture");
inline constexpr std::wstring_view kMmcssModeDistribution(L"Distribution");
inline constexpr std::wstring_view kMmcssProfileGame(L"Games");
inline constexpr std::wstring_view kMmcssProfilePlayback(L"Playback");
inline constexpr std::wstring_view kMmcssProfileProAudio(L"Pro Audio");
inline constexpr std::wstring_view kMmcssProfileWindowsManager(L"Window Manager");

XAMP_MAKE_ENUM(MmcssThreadPriority,
	MMCSS_THREAD_PRIORITY_VERYLOW = -2,
	MMCSS_THREAD_PRIORITY_LOW,
	MMCSS_THREAD_PRIORITY_NORMAL,
	MMCSS_THREAD_PRIORITY_HIGH,
	MMCSS_THREAD_PRIORITY_CRITICAL)

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN

