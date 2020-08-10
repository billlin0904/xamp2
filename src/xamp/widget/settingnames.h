//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/str_utilts.h>

inline constexpr ConstLatin1String kAppSettingLang{ "AppSettings/lang" };

inline constexpr ConstLatin1String kAppSettingPreventSleep{ "AppSettings/preventSleep" };
inline constexpr ConstLatin1String kAppSettingDeviceType{ "AppSettings/deviceType" };
inline constexpr ConstLatin1String kAppSettingDeviceId{ "AppSettings/deviceId" };
inline constexpr ConstLatin1String kAppSettingWidth{ "AppSettings/width" };
inline constexpr ConstLatin1String kAppSettingHeight{ "AppSettings/height" };
inline constexpr ConstLatin1String kAppSettingVolume{ "AppSettings/volume" };
inline constexpr ConstLatin1String kAppSettingOrder{ "AppSettings/order" };
inline constexpr ConstLatin1String kAppSettingNightMode{ "AppSettings/nightMode" };
inline constexpr ConstLatin1String kAppSettingBackgroundColor{ "AppSettings/theme/backgroundColor" };
inline constexpr ConstLatin1String kAppSettingEnableBlur{ "AppSettings/theme/enableBlur" };
inline constexpr ConstLatin1String kAppSettingMusicFilePath{ "AppSettings/musicFilePath" };

inline constexpr ConstLatin1String kAppSettingResamplerEnable{ "AppSettings/soxr/enable" };
inline constexpr ConstLatin1String kAppSettingSoxrSettingName{ "AppSettings/soxr/userSettingName" };

inline constexpr ConstLatin1String kSoxrResampleSampleRate{ "resampleSampleRate" };
inline constexpr ConstLatin1String kSoxrEnableSteepFilter{ "enableSteepFilter" };
inline constexpr ConstLatin1String kSoxrQuality{ "quality" };
inline constexpr ConstLatin1String kSoxrPhase{ "phase" };
inline constexpr ConstLatin1String kSoxrPassBand{ "passBand" };
inline constexpr ConstLatin1String kSoxrDefaultSettingName{ "default" };

inline constexpr ConstLatin1String kEnableEQ{ "AppSettings/enableEQ" };
inline constexpr ConstLatin1String kEQName{ "AppSettings/EQName" };

inline constexpr ConstLatin1String kLyricsFontSize{ "AppSettings/lyrics/fontSize" };

inline constexpr ConstLatin1String kGainStr("Gain");
inline constexpr ConstLatin1String kDbStr("dB");
inline constexpr ConstLatin1String kQStr("Q");