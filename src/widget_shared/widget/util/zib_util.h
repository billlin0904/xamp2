//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QByteArray>
#include <vector>
#include <expected>

#include <widget/widget_shared.h>

XAMP_MAKE_ENUM(GzipDecompressError,
	GZIP_COMPRESS_ERROR_UNKNOWN,
	GZIP_COMPRESS_ERROR_EMPTY_INPUT,
	GZIP_COMPRESS_ERROR_BAD_DATA)

XAMP_MAKE_ENUM(CompressType,
	COMPRESS_GZIP,
	COMPRESS_DEFLATE)

std::expected<QByteArray, GzipDecompressError> gzipCompress(const QByteArray& data, CompressType compress_type = CompressType::COMPRESS_DEFLATE);

std::expected<QByteArray, GzipDecompressError> gzipDecompress(const QByteArray& data);

std::expected<std::string, GzipDecompressError> gzipDecompress(const uint8_t* in_data, size_t in_size);
