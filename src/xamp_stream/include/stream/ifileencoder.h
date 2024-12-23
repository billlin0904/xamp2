//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/anymap.h>

#include <fstream>
#include <base/fs.h>

#include <functional>

XAMP_STREAM_NAMESPACE_BEGIN

namespace FileEncoderConfig {
    constexpr static auto kInputFilePath = std::string_view("InputFilePath");
    constexpr static auto kOutputFilePath = std::string_view("OutputFilePath");
	constexpr static auto kCodecId = std::string_view("CodecId");
    constexpr static auto kBitRate = std::string_view("BitRate");
	constexpr static auto kEncodingProfile = std::string_view("EncodingProfile");
};

class XAMP_STREAM_API XAMP_NO_VTABLE IFileEncodeWriter {
public:
	XAMP_BASE_CLASS(IFileEncodeWriter)

	virtual int32_t Write(const uint8_t* buf, int32_t size) = 0;

	virtual int64_t Seek(int64_t offset, int whence) = 0;
protected:
	IFileEncodeWriter() = default;
};

class XAMP_STREAM_API XAMP_NO_VTABLE IFileEncoder {
public:
	XAMP_BASE_CLASS(IFileEncoder)

	virtual void Start(const AnyMap& config, const std::shared_ptr<IFileEncodeWriter> &writer = std::shared_ptr<IFileEncodeWriter>()) = 0;

	virtual void Encode(const std::stop_token &stop_token, std::function<bool(uint32_t)> const& progress) = 0;

protected:
	IFileEncoder() = default;
};

XAMP_STREAM_NAMESPACE_END
