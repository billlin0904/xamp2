//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/math.h>

namespace xamp::base {

class XAMP_BASE_API SipHash final {
public:
	explicit SipHash(uint64_t k0 = 0, uint64_t k1 = 0);

    void Update(const std::string& x);

    void Update(const char* data, uint64_t size);

    void Finalize();

    uint64_t GetHash();

private:
	uint64_t v0_;
	uint64_t v1_;
	uint64_t v2_;
	uint64_t v3_;
	uint64_t count_;

	union {
		uint64_t current_word;
		uint8_t current_bytes[8];
	};
};

}