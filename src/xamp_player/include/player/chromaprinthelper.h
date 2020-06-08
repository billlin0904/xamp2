//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <tuple>

#include <player/player.h>

namespace xamp::player {

XAMP_PLAYER_API std::tuple<double, std::vector<uint8_t>> ReadFingerprint(std::wstring const & file_path,
                                                                         std::wstring const & file_ext,
                                                                         std::function<bool(uint32_t)> &&progress);

}

