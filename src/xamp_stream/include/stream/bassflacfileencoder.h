//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/ifileencoder.h>

#include <base/align_ptr.h>
#include <base/uuidof.h>

namespace xamp::stream {

class BassFlacFileEncoder final : public IFileEncoder {
	DECLARE_XAMP_MAKE_CLASS_UUID(BassFlacFileEncoder, "C4E07DD2-9296-49B2-99B7-7EEEED3A2046")

public:
	BassFlacFileEncoder();

	XAMP_PIMPL(BassFlacFileEncoder)

	void Start(Path const& input_file_path, Path const& output_file_path, std::wstring const& command) override;

	void Encode(std::function<bool(uint32_t)> const& progress) override;

private:
	class BassFlacFileEncoderImpl;
	AlignPtr<BassFlacFileEncoderImpl> impl_;
};

}

