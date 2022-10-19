//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <base/stl.h>
#include <base/encodingprofile.h>
#include <stream/ifileencoder.h>

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

class BassAACFileEncoder final
	: public IFileEncoder {
public:
	BassAACFileEncoder();

	XAMP_PIMPL(BassAACFileEncoder)

	void Start(Path const& input_file_path, Path const& output_file_path, std::wstring const& command) override;

	void Encode(std::function<bool(uint32_t)> const& progress) override;

    void SetEncodingProfile(const EncodingProfile& profile);

    Vector<EncodingProfile> GetAvailableEncodingProfile();

private:
	class BassAACFileEncoderImpl;
	AlignPtr<BassAACFileEncoderImpl> impl_;
};

}
