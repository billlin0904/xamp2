//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/ifileencoder.h>

#include <base/align_ptr.h>
#include <base/stl.h>
#include <base/uuidof.h>
#include <base/encodingprofile.h>

#ifdef XAMP_OS_MAC

namespace xamp::stream {

 enum EncodingAudioObjectType {
     // "AAC Profile" MPEG-2 Low-complexity (LC) combined with MPEG-4 Perceptual Noise Substitution (PNS)
     ENCODING_AAC_LC = 2,
     // AAC LC + SBR (Spectral Band Replication)
     ENCODING_HE_AAC = 5,
     // "Low Delay Profile" used for real-time communication
     ENCODING_AAC_LD = 23,
     // AAC LC + SBR + PS (Parametric Stereo)
     ENCODING_HE_AAC_V2 = 29,
     // Enhanced Low Delay
     ENCODING_AAC_ELD = 39,
 };

class BassAACFileEncoder final : public IFileEncoder {
    DECLARE_XAMP_MAKE_CLASS_UUID(BassAACFileEncoder, "2D7F4DC9-7AE4-426E-90B1-309CBFE61863");

public:
	BassAACFileEncoder();

	XAMP_PIMPL(BassAACFileEncoder)

    void Start(const AnyMap& config) override;

	void Encode(std::function<bool(uint32_t)> const& progress) override;

    void SetEncodingProfile(const EncodingProfile& profile);

    static Vector<EncodingProfile> GetAvailableEncodingProfile();

private:
	class BassAACFileEncoderImpl;
	AlignPtr<BassAACFileEncoderImpl> impl_;
};

}

#endif
