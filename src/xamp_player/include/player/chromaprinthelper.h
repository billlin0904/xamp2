//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>

#include <player/player.h>

namespace xamp::player {

XAMP_PALYER_API std::vector<uint8_t> ReadFingerprint(const std::wstring& file_path);

}

