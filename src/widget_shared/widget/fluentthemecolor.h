//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QColor>
#include <QString>

enum class FluentThemeColor {
    YELLOW_GOLD = 0xFFB900,
    GOLD = 0xFF8C00,
    ORANGE_BRIGHT = 0xF7630C,
    ORANGE_DARK = 0xCA5010,
    RUST = 0xDA3B01,
    PALE_RUST = 0xEF6950,
    BRICK_RED = 0xD13438,
    MOD_RED = 0xFF4343,
    PALE_RED = 0xE74856,
    RED = 0xE81123,
    ROSE_BRIGHT = 0xEA005E,
    ROSE = 0xC30052,
    PLUM_LIGHT = 0xE3008C,
    PLUM = 0xBF0077,
    ORCHID_LIGHT = 0xBF0077,
    ORCHID = 0x9A0089,
    DEFAULT_BLUE = 0x0078D7,
    NAVY_BLUE = 0x0063B1,
    PURPLE_SHADOW = 0x8E8CD8,
    PURPLE_SHADOW_DARK = 0x6B69D6,
    IRIS_PASTEL = 0x8764B8,
    IRIS_SPRING = 0x744DA9,
    VIOLET_RED_LIGHT = 0xB146C2,
    VIOLET_RED = 0x881798,
    COOL_BLUE_BRIGHT = 0x0099BC,
    COOL_BLUR = 0x2D7D9A,
    SEAFOAM = 0x00B7C3,
    SEAFOAM_TEAL = 0x038387,
    MINT_LIGHT = 0x00B294,
    MINT_DARK = 0x018574,
    TURF_GREEN = 0x00CC6A,
    SPORT_GREEN = 0x10893E,
    GRAY = 0x7A7574,
    GRAY_BROWN = 0x5D5A58,
    STEAL_BLUE = 0x68768A,
    METAL_BLUE = 0x515C6B,
    PALE_MOSS = 0x567C73,
    MOSS = 0x486860,
    MEADOW_GREEN = 0x498205,
    GREEN = 0x107C10,
    OVERCAST = 0x767676,
    STORM = 0x4C4A48,
    BLUE_GRAY = 0x69797E,
    GRAY_DARK = 0x4A5459,
    LIDDY_GREEN = 0x647C64,
    SAGE = 0x525E54,
    CAMOUFLAGE_DESERT = 0x847545,
    CAMOUFLAGE = 0x7E735F
};

inline QColor toQColor(FluentThemeColor theme_color) {
    return {static_cast<QRgb>(theme_color)};
}

