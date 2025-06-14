//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/ifileencoder.h>
#include <base/uuidof.h>

XAMP_STREAM_NAMESPACE_BEGIN

class LibAbFileEncoder final : public IFileEncoder {
	XAMP_DECLARE_MAKE_CLASS_UUID(AlacFileEncoder, "85335F36-A582-4B72-B0E7-3EFFFE35A5DB")

public:
	LibAbFileEncoder();

	XAMP_PIMPL(LibAbFileEncoder)

	void Start(const AnyMap& config,
		const std::shared_ptr<FastIOStream>& file) override;

	void Encode(std::function<bool(uint32_t)> const& progress = nullptr,
		const std::stop_token& stop_token = std::stop_token()) override;

private:
	class LibAbFileEncoderImpl;
	ScopedPtr<LibAbFileEncoderImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

