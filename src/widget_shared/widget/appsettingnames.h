//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/util/str_util.h>

#define DECLARE_APP_SETTING_NAME(Name) \
	XAMP_WIDGET_SHARED_EXPORT extern const ConstexprQString k##Name

// App
DECLARE_APP_SETTING_NAME(AppSettingAutoSelectNewDevice);
DECLARE_APP_SETTING_NAME(AppSettingDontShowMeAgainList);
DECLARE_APP_SETTING_NAME(AppSettingAutoCheckForUpdate);
DECLARE_APP_SETTING_NAME(AppSettingLastTabName);
DECLARE_APP_SETTING_NAME(AppSettingLang);
DECLARE_APP_SETTING_NAME(AppSettingDeviceType);
DECLARE_APP_SETTING_NAME(AppSettingDeviceId);
DECLARE_APP_SETTING_NAME(AppSettingGeometry);
DECLARE_APP_SETTING_NAME(AppSettingScreenNumber);
DECLARE_APP_SETTING_NAME(AppSettingWindowState);
DECLARE_APP_SETTING_NAME(AppSettingVolume);
DECLARE_APP_SETTING_NAME(AppSettingIsMuted);
DECLARE_APP_SETTING_NAME(AppSettingOrder);
DECLARE_APP_SETTING_NAME(AppSettingEnableFadeOut);
DECLARE_APP_SETTING_NAME(AppSettingUseNativeDSDMode);
DECLARE_APP_SETTING_NAME(AppSettingPlaylistColumnName);
DECLARE_APP_SETTING_NAME(AppSettingAlbumPlaylistColumnName);
DECLARE_APP_SETTING_NAME(AppSettingFileSystemPlaylistColumnName);
DECLARE_APP_SETTING_NAME(AppSettingCdPlaylistColumnName);
DECLARE_APP_SETTING_NAME(AppSettingMyMusicFolderPath);
DECLARE_APP_SETTING_NAME(AppSettingLastOpenFolderPath);
DECLARE_APP_SETTING_NAME(AppSettingTheme);
DECLARE_APP_SETTING_NAME(AppSettingMinimizeToTrayAsk);
DECLARE_APP_SETTING_NAME(AppSettingMinimizeToTray);
DECLARE_APP_SETTING_NAME(AppSettingBackgroundColor);
DECLARE_APP_SETTING_NAME(AppSettingCachePath);
DECLARE_APP_SETTING_NAME(AppSettingEnableShortcut);
DECLARE_APP_SETTING_NAME(AppSettingEnterFullScreen);
DECLARE_APP_SETTING_NAME(AppSettingEnableSandboxMode);
DECLARE_APP_SETTING_NAME(AppSettingEnableDebugStackTrace);
DECLARE_APP_SETTING_NAME(AppSettingHideNaviBar);
DECLARE_APP_SETTING_NAME(AppSettingLastPlaylistTabIndex);
DECLARE_APP_SETTING_NAME(AppSettingEnableTray);
DECLARE_APP_SETTING_NAME(AppSettingFileSystemLastOpenPath);

// Waveform
DECLARE_APP_SETTING_NAME(AppSettingWaveformColor);

// Resampler
DECLARE_APP_SETTING_NAME(AppSettingResamplerEnable);
DECLARE_APP_SETTING_NAME(AppSettingResamplerType);
DECLARE_APP_SETTING_NAME(AppSettingSoxrSettingName);

// Spectrum
DECLARE_APP_SETTING_NAME(AppSettingEnableSpectrum);
DECLARE_APP_SETTING_NAME(AppSettingSpectrumStyles);
DECLARE_APP_SETTING_NAME(AppSettingWindowType);
DECLARE_APP_SETTING_NAME(ResampleSampleRate);

// Soxr
DECLARE_APP_SETTING_NAME(Soxr);
DECLARE_APP_SETTING_NAME(SoxrQuality);
DECLARE_APP_SETTING_NAME(SoxrStopBand);
DECLARE_APP_SETTING_NAME(SoxrPassBand);
DECLARE_APP_SETTING_NAME(SoxrPhase);
DECLARE_APP_SETTING_NAME(SoxrRollOffLevel);
DECLARE_APP_SETTING_NAME(SoxrDefaultSettingName);

// R8Brain
DECLARE_APP_SETTING_NAME(R8Brain);
DECLARE_APP_SETTING_NAME(Src);
DECLARE_APP_SETTING_NAME(AppSettingEnableEQ);
DECLARE_APP_SETTING_NAME(AppSettingEQName);
DECLARE_APP_SETTING_NAME(LyricsFontSize);

// Log
DECLARE_APP_SETTING_NAME(Log);
DECLARE_APP_SETTING_NAME(LogMinimumLevel);
DECLARE_APP_SETTING_NAME(LogDefault);
DECLARE_APP_SETTING_NAME(LogOverride);

DECLARE_APP_SETTING_NAME(AppSettingReplayGainMode);
DECLARE_APP_SETTING_NAME(AppSettingYtMusicPoToken);

