//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <base/align_ptr.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

class XAMP_PALYER_API Chromaprint {
public:
	explicit Chromaprint();

	~Chromaprint();

	XAMP_DISABLE_COPY(Chromaprint)

	void Start(int32_t sample_rate, int32_t num_channels, int32_t num_buffer_frames);

	int Feed(const float* data, int size) const;

	int Finish() const;

	std::vector<uint8_t> GetFingerprint() const;
private:
	class ChromaprintImpl;
	AlignPtr<ChromaprintImpl> impl_;
};

}