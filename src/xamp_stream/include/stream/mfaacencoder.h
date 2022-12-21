//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/ifileencoder.h>

#ifdef XAMP_OS_WIN

#include <base/align_ptr.h>
#include <base/audioformat.h>
#include <base/uuidof.h>
#include <base/stl.h>
#include <base/encodingprofile.h>

namespace xamp::stream {

enum AACProfileLevel {
	AAC_PROFILE_L2 = 0x29,
	AAC_PROFILE_L4 = 0x2A,
	AAC_PROFILE_L5 = 0x2B,
	AAC_HEV1_PROFILE_L2 = 0x2C,
	AAC_HEV1_PROFILE_L4 = 0x2E,
	AAC_HEV1_PROFILE_L5 = 0x2F,
	AAC_HEV2_PROFILE_L2 = 0x30,
	AAC_HEV2_PROFILE_L3 = 0x31,
	AAC_HEV2_PROFILE_L4 = 0x32,
	AAC_HEV2_PROFILE_L5 = 0x33
};

XAMP_MAKE_ENUM(AACPayloadType,
	PAYLOAD_AAC_RAW,
	PAYLOAD_AAC_ADTS,
	PAYLOAD_AAC_ADIF,
	PAYLOAD_AAC_LOAS_AND_LATM)

class XAMP_STREAM_API MFAACFileEncoder final : public IFileEncoder {
	XAMP_DECLARE_MAKE_CLASS_UUID(MFAACFileEncoder, "1817BCF4-F2CF-48CB-AFF4-A3D5B8989FDA")

public:
	MFAACFileEncoder();

	XAMP_PIMPL(MFAACFileEncoder)

	void Start(const AnyMap& config) override;

	void Encode(std::function<bool(uint32_t)> const& progress) override;

	void SetEncodingProfile(const EncodingProfile& profile);

	static Vector<EncodingProfile> GetAvailableEncodingProfile();
private:
	class MFAACFileEncoderImpl;
	AlignPtr<MFAACFileEncoderImpl> impl_;
};

}
#endif
