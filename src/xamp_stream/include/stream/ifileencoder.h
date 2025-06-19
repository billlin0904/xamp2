//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/anymap.h>

#include <fstream>
#include <base/fastiostream.h>

#include <functional>

XAMP_STREAM_NAMESPACE_BEGIN

namespace FileEncoderConfig {
	inline constexpr static auto kInputFilePath =
		std::string_view("InputFilePath");
	inline constexpr static auto kOutputFilePath =
		std::string_view("OutputFilePath");
	inline constexpr static auto kCodecId =
		std::string_view("CodecId");
	inline constexpr static auto kBitRate =
		std::string_view("BitRate");
	inline constexpr static auto kEncodingProfile =
		std::string_view("EncodingProfile");
};

class XAMP_STREAM_API XAMP_NO_VTABLE IFileEncoder {
public:
	XAMP_BASE_CLASS(IFileEncoder)

	virtual void Start(const AnyMap& config,
		const std::shared_ptr<FastIOStream> & file) = 0;

	virtual void Encode(std::function<bool(uint32_t)> const& progress = nullptr,
		const std::stop_token& stop_token = std::stop_token()) = 0;

protected:
	IFileEncoder() = default;
};

XAMP_STREAM_NAMESPACE_END
