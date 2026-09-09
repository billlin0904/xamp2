//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/enum.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

XAMP_BASE_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(GzipDecompressError,
	GZIP_COMPRESS_ERROR_UNKNOWN,
	GZIP_COMPRESS_ERROR_EMPTY_INPUT,
	GZIP_COMPRESS_ERROR_BAD_DATA)

XAMP_MAKE_ENUM(CompressType,
	COMPRESS_GZIP,
	COMPRESS_DEFLATE)

XAMP_BASE_API void loadLibdeflate();

XAMP_BASE_API std::expected<std::vector<uint8_t>, GzipDecompressError> gzipCompress(
	const uint8_t* in_data,
	size_t in_size,
	CompressType compress_type = CompressType::COMPRESS_DEFLATE);

XAMP_BASE_API std::expected<std::vector<uint8_t>, GzipDecompressError> gzipCompress(
	const std::vector<uint8_t>& data,
	CompressType compress_type = CompressType::COMPRESS_DEFLATE);

XAMP_BASE_API std::expected<std::vector<uint8_t>, GzipDecompressError> gzipDecompressBytes(
	const uint8_t* in_data,
	size_t in_size);

XAMP_BASE_API std::expected<std::vector<uint8_t>, GzipDecompressError> gzipDecompressBytes(
	const std::vector<uint8_t>& data);

XAMP_BASE_API std::expected<std::string, GzipDecompressError> gzipDecompress(
	const uint8_t* in_data,
	size_t in_size);

XAMP_BASE_API std::expected<std::string, GzipDecompressError> gzipDecompress(
	const std::vector<uint8_t>& data);

XAMP_BASE_NAMESPACE_END
