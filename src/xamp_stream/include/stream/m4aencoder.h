//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/ifileencoder.h>
#include <base/uuidof.h>

XAMP_STREAM_NAMESPACE_BEGIN

class M4AFileEncoder final : public IFileEncoder {
	XAMP_DECLARE_MAKE_CLASS_UUID(AlacFileEncoder, "85335F36-A582-4B72-B0E7-3EFFFE35A5DB")

public:
	M4AFileEncoder();

	XAMP_PIMPL(M4AFileEncoder)

	void Start(const AnyMap& config, const std::shared_ptr<IIoContext>& io_context) override;

	void Encode(const std::stop_token& stop_token, std::function<bool(uint32_t)> const& progress) override;

private:
	class M4AFileEncoderImpl;
	ScopedPtr<M4AFileEncoderImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

