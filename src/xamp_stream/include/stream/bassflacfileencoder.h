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

class BassFlacFileEncoder final : public IFileEncoder {
	XAMP_DECLARE_MAKE_CLASS_UUID(BassFlacFileEncoder, "C4E07DD2-9296-49B2-99B7-7EEEED3A2046")

public:
	BassFlacFileEncoder();

	XAMP_PIMPL(BassFlacFileEncoder)

	void Start(const AnyMap& config) override;

	void Encode(std::function<bool(uint32_t)> const& progress) override;

private:
	class BassFlacFileEncoderImpl;
	ScopedPtr<BassFlacFileEncoderImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

