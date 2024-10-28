//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/ifileencoder.h>
#include <base/uuidof.h>

XAMP_STREAM_NAMESPACE_BEGIN

class AlacFileEncoder final : public IFileEncoder {
	XAMP_DECLARE_MAKE_CLASS_UUID(AlacFileEncoder, "85335F36-A582-4B72-B0E7-3EFFFE35A5DB")

public:
	AlacFileEncoder();

	XAMP_PIMPL(AlacFileEncoder)

	void Start(const AnyMap& config) override;

	void Encode(std::function<bool(uint32_t)> const& progress) override;
private:
	class AlacFileEncoderImpl;
	ScopedPtr<AlacFileEncoderImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

