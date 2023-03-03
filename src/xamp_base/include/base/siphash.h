//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#ifdef XAMP_USE_BENCHMAKR
#include <string>
#include <base/base.h>
#include <base/math.h>

namespace xamp::base {

class XAMP_BASE_API SipHash final {
public:
	SipHash();

	constexpr SipHash(
		uint64_t k0,
		uint64_t k1
	);

	constexpr void Clear(uint64_t k0 = 0, uint64_t k1 = 0);

    void Update(const std::string& x);

    void Update(const char* data, size_t size);

	template <typename CharT>
	void Update(const std::basic_string< CharT> &str) {
		Update(reinterpret_cast<const char*>(str.data()), str.size() * sizeof(CharT));
	}

    void Finalize();

    size_t GetHash();

	static uint64_t GetHash(const std::string& x, uint64_t k0, uint64_t k1);

private:
	uint64_t v0_;
	uint64_t v1_;
	uint64_t v2_;
	uint64_t v3_;
	uint64_t count_;

	union {
		uint64_t current_word;
		uint8_t current_bytes[8]{};
	};
};

}
#endif