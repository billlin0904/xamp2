#include <widget/appsettingnames.h>

#define IMPL_APP_SETTING_NAME(Name, Path) \
	const ConstLatin1String k##Name { Path }

IMPL_APP_SETTING_NAME(AppSettingAutoCheckForUpdate, "AppSettings/autoCheckForUpdate" );
IMPL_APP_SETTING_NAME(AppSettingLastTabName, "AppSettings/lastTabName");
IMPL_APP_SETTING_NAME(AppSettingLang, "AppSettings/lang");

const ConstLatin1String kAppSettingDeviceType{ "AppSettings/deviceType" };
const ConstLatin1String kAppSettingDeviceId{ "AppSettings/deviceId" };
const ConstLatin1String kAppSettingGeometry{ "AppSettings/geometry" };
const ConstLatin1String kAppSettingScreenNumber{ "AppSettings/screenNumber" };
const ConstLatin1String kAppSettingWindowState{ "AppSettings/windowState" };
const ConstLatin1String kAppSettingVolume{ "AppSettings/volume" };
const ConstLatin1String kAppSettingOrder{ "AppSettings/order" };
const ConstLatin1String kAppSettingUseNativeDSDMode{ "AppSettings/useNativeDSDMode" };
const ConstLatin1String kAppSettingUseFramelessWindow{ "AppSettings/useFramelessWindow" };
const ConstLatin1String kAppSettingShowLeftList{ "AppSettings/showLeftList" };
const ConstLatin1String kAppSettingDiscordNotify{ "AppSettings/discordNotify" };
const ConstLatin1String kAppSettingColumnName{ "AppSettings/columnName" };
const ConstLatin1String kAppSettingIsMuted{"AppSettings/isMuted"};
const ConstLatin1String kAppSettingMyMusicFolderPath{ "AppSettings/myMusicFolderPath" };

const ConstLatin1String kAppSettingTheme{ "AppSettings/theme/themeColor" };
const ConstLatin1String kAppSettingEnableBlur{ "AppSettings/theme/enableBlur" };
const ConstLatin1String kAppSettingMinimizeToTrayAsk{ "AppSettings/minimizeToTrayAsk" };
const ConstLatin1String kAppSettingMinimizeToTray{ "AppSettings/minimizeToTray" };
const ConstLatin1String kAppSettingBackgroundColor{ "AppSettings/theme/backgroundColor" };

const ConstLatin1String kAppSettingPodcastCachePath{ "AppSettings/podcastCachePath" };
const ConstLatin1String kAppSettingAlbumImageCacheSize{ "AppSettings/albumImageCacheSize" };
const ConstLatin1String kAppSettingEnableReplayGain{ "AppSettings/enableReplayGain" };
const ConstLatin1String kAppSettingReplayGainMode{ "AppSettings/replayGainMode" };
const ConstLatin1String kAppSettingReplayGainTargetGain{ "AppSettings/replayGainTargetGain" };
const ConstLatin1String kAppSettingReplayGainScanMode{ "AppSettings/replayGainScanMode" };

const ConstLatin1String kAppSettingResamplerEnable{ "AppSettings/enable" };
const ConstLatin1String kAppSettingResamplerType{ "AppSettings/resamplerType" };
const ConstLatin1String kResampleSampleRate{ "resampleSampleRate" };

const ConstLatin1String kAppSettingSoxrSettingName{ "AppSettings/soxr/userSettingName" };

const ConstLatin1String kAppSettingEnableEQ{ "AppSettings/enableEQ" };
const ConstLatin1String kAppSettingEQName{ "AppSettings/EQName" };

const ConstLatin1String kLyricsFontSize{ "AppSettings/lyrics/fontSize" };
const ConstLatin1String kLyricsTextColor{ "AppSettings/lyrics/textColor" };
const ConstLatin1String kLyricsHighLightTextColor{ "AppSettings/lyrics/highLightTextColor" };

const ConstLatin1String kFlacEncodingLevel{ "AppSettings/flacEncodingLevel" };

const ConstLatin1String kEnableBitPerfect{ "AppSettings/enableBitPerfect" };
const ConstLatin1String kEnableBlurCover{ "AppSettings/enableBlurCover" };

const ConstLatin1String kAppSettingEnableSpectrum{ "AppSettings/Spectrum/enable" };;
const ConstLatin1String kAppSettingSpectrumStyles{ "AppSettings/Spectrum/spectrumStyles" };
const ConstLatin1String kAppSettingWindowType{ "AppSettings/Spectrum/windowType" };


const ConstLatin1String kSoxr{ "Soxr" };
const ConstLatin1String kSoxrDefaultSettingName{ "default" };
const ConstLatin1String kSoxrEnableSteepFilter{ "enableSteepFilter" };
const ConstLatin1String kSoxrQuality{ "quality" };
const ConstLatin1String kSoxrStopBand{ "stopBand" };
const ConstLatin1String kSoxrPassBand{ "passBand" };
const ConstLatin1String kSoxrPhase{ "phase" };
const ConstLatin1String kSoxrRollOffLevel{ "rolloffLevel" };

const ConstLatin1String kR8Brain{ "R8Brain" };

IMPL_APP_SETTING_NAME(PCM2DSD, "Pcm2Dsd");
IMPL_APP_SETTING_NAME(PCM2DSDDsdTimes, "Pcm2DsdTimes");
IMPL_APP_SETTING_NAME(EnablePcm2Dsd, "AppSettings/enablePcm2Dsd");

const ConstLatin1String kLog{ "Log" };
const ConstLatin1String kLogMinimumLevel{ "MinimumLevel" };
const ConstLatin1String kLogDefault{ "Default" };
const ConstLatin1String kLogOverride{ "Override" };
