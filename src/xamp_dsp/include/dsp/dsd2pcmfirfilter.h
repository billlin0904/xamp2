//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>

#include <dsp/dsp.h>
#include <dsp/dsd2pcmsettings.h>

namespace xamp::dsp {

using namespace base;

class XAMP_DSP_API Dsd2PcmFirFilter {
public:
	Dsd2PcmFirFilter();

	~Dsd2PcmFirFilter();	

	void Initial(const CoefsFirTable& fir_table, int32_t decimation);

	int32_t GetDelay() const;

	int32_t operator()(float* pcm, const uint8_t* dsd, int32_t samples);
private:
	int32_t decimation_;
	int32_t fir_index_;
	size_t fir_length_;
	CoefsFirTable fir_table_;
	AlignBufferPtr<uint8_t> fir_buffer_;
};

}


