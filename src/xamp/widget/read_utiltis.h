//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <tuple>

#include <base/align_ptr.h>
#include <base/metadata.h>
#include <player/player.h>

using namespace xamp::base;
using namespace xamp::player;

std::tuple<double, std::vector<uint8_t>> ReadFingerprint(
	std::wstring const & file_path,
    std::function<bool(uint32_t)> const & progress);
	
std::tuple<double, double> ReadFileLUFS(std::wstring const& file_path,
	std::function<bool(uint32_t)> const& progress);

void Export2WaveFile(std::wstring const& file_path,
	std::wstring const& output_file_path,
	std::function<bool(uint32_t)> const& progress,
	Metadata const &metadata,
	bool enable_compressor = true);

void Export2WaveFile(std::wstring const& file_path,
	std::wstring const& output_file_path,
	std::function<bool(uint32_t)> const& progress,
	Metadata const& metadata,
	uint32_t output_sample_rate,
	AlignPtr<ISampleRateConverter> &converter);

void EncodeFlacFile(std::wstring const& file_path,
    std::wstring const& output_file_path,
    std::wstring const& command,
    std::function<bool(uint32_t)> const& progress,
    Metadata const& metadata);

