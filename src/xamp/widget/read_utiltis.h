//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <tuple>
#include <utility>

#include <base/align_ptr.h>
#include <base/metadata.h>
#include <base/audioformat.h>

#include <stream/iaudioprocessor.h>
#include <stream/isamplerateconverter.h>

using namespace xamp::base;
using namespace xamp::stream;

namespace read_utiltis {

double readAll(std::wstring const& file_path,
               std::function<bool(uint32_t)> const& progress,
               std::function<void(AudioFormat const&)> const& prepare,
               std::function<void(float const*, uint32_t)> const& dsp_process = nullptr,
               uint64_t max_duration = (std::numeric_limits<uint64_t>::max)());
	
std::tuple<double, double> readFileLUFS(std::wstring const& file_path,
    std::function<bool(uint32_t)> const& progress,
    uint64_t max_duration = (std::numeric_limits<uint64_t>::max)());

void export2WaveFile(std::wstring const& file_path,
	std::wstring const& output_file_path,
	std::function<bool(uint32_t)> const& progress,
	Metadata const &metadata,
	bool enable_compressor = true);

void export2WaveFile(std::wstring const& file_path,
	std::wstring const& output_file_path,
	std::function<bool(uint32_t)> const& progress,
	Metadata const& metadata,
	uint32_t output_sample_rate,
	AlignPtr<ISampleRateConverter> &converter);

void encodeFlacFile(std::wstring const& file_path,
    std::wstring const& output_file_path,
    std::wstring const& command,
    std::function<bool(uint32_t)> const& progress,
    Metadata const& metadata);

}
