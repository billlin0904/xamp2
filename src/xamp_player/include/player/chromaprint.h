//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <base/align_ptr.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

class XAMP_PLAYER_API Chromaprint {
public:
	explicit Chromaprint();

	XAMP_PIMPL(Chromaprint)

    static void LoadChromaprintLib();

    void Start(uint32_t sample_rate, uint32_t num_channels);

    int32_t Feed(int16_t const * data, uint32_t size) const;

    int32_t Finish() const;

	std::vector<uint8_t> GetFingerprint() const;
private:
	class ChromaprintImpl;
    AlignPtr<ChromaprintImpl> impl_;
};

}
