//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>

#include <player/player.h>

namespace xamp::player {

struct XAMP_PLAYER_API Fingerprint {
	double duration{ 0 };
	std::vector<uint8_t> fingerprint;
};

XAMP_PLAYER_API Fingerprint ReadFingerprint(const std::wstring& file_path);

}

