//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <stream/ifileencoder.h>

namespace xamp::stream {

class BassAACFileEncoder final
	: public IFileEncoder {
public:
	BassAACFileEncoder();

	XAMP_PIMPL(BassAACFileEncoder)

	void Start(Path const& input_file_path, Path const& output_file_path, std::wstring const& command) override;

	void Encode(std::function<bool(uint32_t)> const& progress) override;

private:
	class BassAACFileEncoderImpl;
	AlignPtr<BassAACFileEncoderImpl> impl_;
};

}