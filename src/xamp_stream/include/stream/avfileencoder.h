//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/ifileencoder.h>

#include <base/audioformat.h>
#include <base/memory.h>
#include <base/align_ptr.h>
#include <base/uuidof.h>

namespace xamp::stream {

XAMP_MAKE_ENUM(AvCodecId,
	AV_CODEC_ID_AAC,
	AV_CODEC_ID_MP3,
	AV_CODEC_ID_WAV,
	AV_CODEC_ID_FLAC)

class AvFileEncoder final : public IFileEncoder {
	DECLARE_XAMP_MAKE_CLASS_UUID(AvFileEncoder, "89DFACBB-6798-447F-B195-B52DBBFC3793")

public:
	AvFileEncoder();

	XAMP_PIMPL(AvFileEncoder)

	void Start(const AnyMap& config) override;

	void Encode(std::function<bool(uint32_t)> const& progress) override;

private:
	class AvFileEncoderImpl;
	AlignPtr<AvFileEncoderImpl> impl_;
};

}

