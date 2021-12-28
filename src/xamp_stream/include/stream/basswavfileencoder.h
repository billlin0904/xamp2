//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <stream/ifileencoder.h>

#ifdef XAMP_OS_WIN

namespace xamp::stream {

class BassWavFileEncoder final
	: public IFileEncoder {
public:
	BassWavFileEncoder();

	XAMP_PIMPL(BassWavFileEncoder)

	void Start(std::wstring const& input_file_path, std::wstring const& output_file_path, std::wstring const& command) override;

	void Encode(std::function<bool(uint32_t)> const& progress) override;

private:
	class BassWavFileEncoderImpl;
	AlignPtr<BassWavFileEncoderImpl> impl_;
};

}

#endif
