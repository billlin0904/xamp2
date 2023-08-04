#include <widget/appsettingnames.h>

#define IMPL_APP_SETTING_NAME(Name, Path) \
	const ConstLatin1String k##Name { Path }

IMPL_APP_SETTING_NAME(AppSettingAutoCheckForUpdate, "AppSettings/autoCheckForUpdate" );
IMPL_APP_SETTING_NAME(AppSettingDontShowMeAgainList, "AppSettings/dontShowMeAgain");

IMPL_APP_SETTING_NAME(AppSettingDefaultDir, "AppSettings/defaultDir");
IMPL_APP_SETTING_NAME(AppSettingLastTabName, "AppSettings/lastTabName");
IMPL_APP_SETTING_NAME(AppSettingLang, "AppSettings/lang");
IMPL_APP_SETTING_NAME(AppSettingDeviceType, "AppSettings/deviceType");
IMPL_APP_SETTING_NAME(AppSettingDeviceId, "AppSettings/deviceId");
IMPL_APP_SETTING_NAME(AppSettingGeometry, "AppSettings/geometry");
IMPL_APP_SETTING_NAME(AppSettingScreenNumber, "AppSettings/screenNumber");
IMPL_APP_SETTING_NAME(AppSettingWindowState, "AppSettings/windowState");
IMPL_APP_SETTING_NAME(AppSettingVolume, "AppSettings/volume");
IMPL_APP_SETTING_NAME(AppSettingOrder, "AppSettings/order");
IMPL_APP_SETTING_NAME(AppSettingUseNativeDSDMode, "AppSettings/useNativeDSDMode");
IMPL_APP_SETTING_NAME(AppSettingUseFramelessWindow, "AppSettings/useFramelessWindow");
IMPL_APP_SETTING_NAME(AppSettingShowLeftList, "AppSettings/showLeftList");
IMPL_APP_SETTING_NAME(AppSettingDiscordNotify, "AppSettings/discordNotify");
IMPL_APP_SETTING_NAME(AppSettingPlaylistColumnName, "AppSettings/columnName");
IMPL_APP_SETTING_NAME(AppSettingPodcastPlaylistColumnName, "AppSettings/podcastColumnName");
IMPL_APP_SETTING_NAME(AppSettingAlbumPlaylistColumnName, "AppSettings/albumColumnName");
IMPL_APP_SETTING_NAME(AppSettingFileSystemPlaylistColumnName, "AppSettings/fileSystemColumnName");
IMPL_APP_SETTING_NAME(AppSettingCdPlaylistColumnName, "AppSettings/cdColumnName");
IMPL_APP_SETTING_NAME(AppSettingIsMuted,"AppSettings/isMuted");
IMPL_APP_SETTING_NAME(AppSettingMyMusicFolderPath, "AppSettings/myMusicFolderPath");
IMPL_APP_SETTING_NAME(AppSettingEnterFullScreen, "AppSettings/enterFullScreen");

IMPL_APP_SETTING_NAME(AppSettingTheme, "AppSettings/theme/themeColor");
IMPL_APP_SETTING_NAME(AppSettingMinimizeToTrayAsk, "AppSettings/minimizeToTrayAsk");
IMPL_APP_SETTING_NAME(AppSettingMinimizeToTray, "AppSettings/minimizeToTray");
IMPL_APP_SETTING_NAME(AppSettingBackgroundColor, "AppSettings/theme/backgroundColor");

IMPL_APP_SETTING_NAME(AppSettingPodcastCachePath, "AppSettings/podcastCachePath");
IMPL_APP_SETTING_NAME(AppSettingAlbumImageCachePath, "AppSettings/albumImageCacheSPath");
IMPL_APP_SETTING_NAME(AppSettingEnableReplayGain, "AppSettings/enableReplayGain");
IMPL_APP_SETTING_NAME(AppSettingReplayGainMode, "AppSettings/replayGainMode");
IMPL_APP_SETTING_NAME(AppSettingReplayGainTargetGain, "AppSettings/replayGainTargetGain");
IMPL_APP_SETTING_NAME(AppSettingReplayGainTargetLoudnes, "AppSettings/replayGainTargetLoudnes");
IMPL_APP_SETTING_NAME(AppSettingReplayGainScanMode, "AppSettings/replayGainScanMode");

IMPL_APP_SETTING_NAME(AppSettingResamplerEnable, "AppSettings/enableResampler");
IMPL_APP_SETTING_NAME(AppSettingResamplerType, "AppSettings/resamplerType");
IMPL_APP_SETTING_NAME(ResampleSampleRate, "resampleSampleRate");

IMPL_APP_SETTING_NAME(AppSettingSoxrSettingName, "AppSettings/soxr/userSettingName");

IMPL_APP_SETTING_NAME(AppSettingEnableEQ, "AppSettings/enableEQ");
IMPL_APP_SETTING_NAME(AppSettingEQName, "AppSettings/EQName");

IMPL_APP_SETTING_NAME(LyricsFontSize, "AppSettings/lyrics/fontSize");
IMPL_APP_SETTING_NAME(LyricsTextColor, "AppSettings/lyrics/textColor");
IMPL_APP_SETTING_NAME(LyricsHighLightTextColor, "AppSettings/lyrics/highLightTextColor");

IMPL_APP_SETTING_NAME(FlacEncodingLevel, "AppSettings/flacEncodingLevel");

IMPL_APP_SETTING_NAME(EnableBitPerfect, "AppSettings/enableBitPerfect");
IMPL_APP_SETTING_NAME(EnableBlurCover, "AppSettings/enableBlurCover");

IMPL_APP_SETTING_NAME(AppSettingEnableSpectrum, "AppSettings/spectrum/enable");;
IMPL_APP_SETTING_NAME(AppSettingSpectrumStyles, "AppSettings/spectrum/spectrumStyles");
IMPL_APP_SETTING_NAME(AppSettingWindowType, "AppSettings/spectrum/windowType");


IMPL_APP_SETTING_NAME(Soxr, "Soxr");
IMPL_APP_SETTING_NAME(SoxrDefaultSettingName, "default");
IMPL_APP_SETTING_NAME(SoxrQuality, "quality");
IMPL_APP_SETTING_NAME(SoxrStopBand, "stopBand");
IMPL_APP_SETTING_NAME(SoxrPassBand, "passBand");
IMPL_APP_SETTING_NAME(SoxrPhase, "phase");
IMPL_APP_SETTING_NAME(SoxrRollOffLevel, "rolloffLevel");

IMPL_APP_SETTING_NAME(R8Brain, "R8Brain");

IMPL_APP_SETTING_NAME(Log, "Log");
IMPL_APP_SETTING_NAME(LogMinimumLevel, "MinimumLevel");
IMPL_APP_SETTING_NAME(LogDefault, "Default");
IMPL_APP_SETTING_NAME(LogOverride, "Override");

IMPL_APP_SETTING_NAME(AppSettingEnableShortcut, "AppSettings/enableShortcut");
IMPL_APP_SETTING_NAME(AppSettingEnableSandboxMode, "AppSettings/enableSandboxMode");
IMPL_APP_SETTING_NAME(AppSettingEnableDebugStackTrace, "AppSettings/enableDebugStackTrace");