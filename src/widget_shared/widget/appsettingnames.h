//=====================================================================================================================
// Copyright (c) 2018-2021 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/str_utilts.h>

#define DECLARE_APP_SETTING_NAME(Name) \
	XAMP_WIDGET_SHARED_EXPORT extern const ConstLatin1String k##Name

DECLARE_APP_SETTING_NAME(EnablePcm2Dsd);
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
DECLARE_APP_SETTING_NAME(AppSettingUseNativeDSDMode);
DECLARE_APP_SETTING_NAME(AppSettingUseFramelessWindow);
DECLARE_APP_SETTING_NAME(AppSettingDiscordNotify);
DECLARE_APP_SETTING_NAME(AppSettingPlaylistColumnName);
DECLARE_APP_SETTING_NAME(AppSettingPodcastPlaylistColumnName);
DECLARE_APP_SETTING_NAME(AppSettingAlbumPlaylistColumnName);
DECLARE_APP_SETTING_NAME(AppSettingFileSystemPlaylistColumnName);
DECLARE_APP_SETTING_NAME(AppSettingCdPlaylistColumnName);
DECLARE_APP_SETTING_NAME(AppSettingMyMusicFolderPath);
DECLARE_APP_SETTING_NAME(AppSettingLastOpenFolderPath);
DECLARE_APP_SETTING_NAME(AppSettingTheme);
DECLARE_APP_SETTING_NAME(AppSettingMinimizeToTrayAsk);
DECLARE_APP_SETTING_NAME(AppSettingMinimizeToTray);
DECLARE_APP_SETTING_NAME(AppSettingBackgroundColor);
DECLARE_APP_SETTING_NAME(AppSettingPodcastCachePath);
DECLARE_APP_SETTING_NAME(AppSettingEnableReplayGain);
DECLARE_APP_SETTING_NAME(AppSettingEnableReplayGainWriteTag);
DECLARE_APP_SETTING_NAME(AppSettingReplayGainMode);
DECLARE_APP_SETTING_NAME(AppSettingReplayGainTargetGain);
DECLARE_APP_SETTING_NAME(AppSettingReplayGainTargetLoudnes);
DECLARE_APP_SETTING_NAME(AppSettingReplayGainScanMode);
DECLARE_APP_SETTING_NAME(AppSettingResamplerEnable);
DECLARE_APP_SETTING_NAME(AppSettingResamplerType);
DECLARE_APP_SETTING_NAME(AppSettingSoxrSettingName);
DECLARE_APP_SETTING_NAME(EnableBlurCover);
DECLARE_APP_SETTING_NAME(AppSettingEnableSpectrum);
DECLARE_APP_SETTING_NAME(AppSettingSpectrumStyles);
DECLARE_APP_SETTING_NAME(AppSettingWindowType);
DECLARE_APP_SETTING_NAME(ResampleSampleRate);
DECLARE_APP_SETTING_NAME(Soxr);
DECLARE_APP_SETTING_NAME(SoxrQuality);
DECLARE_APP_SETTING_NAME(SoxrStopBand);
DECLARE_APP_SETTING_NAME(SoxrPassBand);
DECLARE_APP_SETTING_NAME(SoxrPhase);
DECLARE_APP_SETTING_NAME(SoxrRollOffLevel);
DECLARE_APP_SETTING_NAME(SoxrDefaultSettingName);
DECLARE_APP_SETTING_NAME(R8Brain);
DECLARE_APP_SETTING_NAME(Src);
DECLARE_APP_SETTING_NAME(EnableBitPerfect);
DECLARE_APP_SETTING_NAME(AppSettingEnableEQ);
DECLARE_APP_SETTING_NAME(AppSettingEQName);
DECLARE_APP_SETTING_NAME(LyricsFontSize);
DECLARE_APP_SETTING_NAME(LyricsTextColor);
DECLARE_APP_SETTING_NAME(LyricsHighLightTextColor);
DECLARE_APP_SETTING_NAME(FlacEncodingLevel);
DECLARE_APP_SETTING_NAME(Log);
DECLARE_APP_SETTING_NAME(LogMinimumLevel);
DECLARE_APP_SETTING_NAME(LogDefault);
DECLARE_APP_SETTING_NAME(LogOverride);
DECLARE_APP_SETTING_NAME(AppSettingAlbumImageCachePath);
DECLARE_APP_SETTING_NAME(AppSettingEnableShortcut);
DECLARE_APP_SETTING_NAME(AppSettingEnterFullScreen);
DECLARE_APP_SETTING_NAME(AppSettingEnableSandboxMode);
DECLARE_APP_SETTING_NAME(AppSettingEnableDebugStackTrace);