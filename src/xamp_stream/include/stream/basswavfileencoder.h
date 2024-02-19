//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/ifileencoder.h>

#include <base/memory.h>
#include <base/uuidof.h>
#include <base/pimplptr.h>

XAMP_STREAM_NAMESPACE_BEGIN

class BassWavFileEncoder final 	: public IFileEncoder {
	XAMP_DECLARE_MAKE_CLASS_UUID(BassWavFileEncoder, "41592357-F396-4222-965C-E5A465E128C1")

public:
	BassWavFileEncoder();

	XAMP_PIMPL(BassWavFileEncoder)

	void Start(const AnyMap& config) override;

	void Encode(std::function<bool(uint32_t)> const& progress) override;

private:
	class BassWavFileEncoderImpl;
	AlignPtr<BassWavFileEncoderImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
