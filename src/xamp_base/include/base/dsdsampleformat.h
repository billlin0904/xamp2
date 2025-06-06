//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(DsdFormat,
    DSD_INT8LSB,
    DSD_INT8MSB,
    DSD_INT8NER8)

XAMP_MAKE_ENUM(DsdModes,
    DSD_MODE_AUTO,
    DSD_MODE_PCM,
    DSD_MODE_NATIVE,
    DSD_MODE_DOP,
    DSD_MODE_DOP_AA,
    DSD_MODE_DSD2PCM)

inline bool IsPcmAudio(DsdModes dsd_mode) {
    return (dsd_mode == DsdModes::DSD_MODE_PCM || dsd_mode == DsdModes::DSD_MODE_DSD2PCM);
}

XAMP_BASE_NAMESPACE_END
