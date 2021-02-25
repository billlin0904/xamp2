//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <tuple>

#include <player/player.h>

namespace xamp::player {

XAMP_PLAYER_API std::tuple<double, std::vector<uint8_t>> ReadFingerprint(
	std::wstring const & file_path,
    std::wstring const & file_ext,
    std::function<bool(uint32_t)> const & progress);
	
XAMP_PLAYER_API std::tuple<double, double> ReadFileLUFS(std::wstring const& file_path,
	std::wstring const& file_ext,
	std::function<bool(uint32_t)> const& progress);

XAMP_PLAYER_API void Export2WaveFile(std::wstring const& file_path,
	std::wstring const& file_ext,
	std::wstring const& output_file_path,
	std::function<bool(uint32_t)> const& progress,
	bool enable_compressor = false);

}

