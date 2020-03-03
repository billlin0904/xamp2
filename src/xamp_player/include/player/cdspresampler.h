//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <base/audioformat.h>
#include <player/player.h>

namespace xamp::player {

class XAMP_PALYER_API CdspResampler {
public:
	CdspResampler();

	XAMP_PIMPL(CdspResampler)

	bool Process(const float* samples, int32_t num_sample, AudioBuffer<int8_t>& buffer);
private:
	class CdspResamplerImpl;
	AlignPtr<CdspResamplerImpl> impl_;
};

}
