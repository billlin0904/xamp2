//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <base/base.h>
#include <base/math.h>

namespace xamp::base {

class XAMP_BASE_API SipHash final {
public:
	explicit SipHash(uint64_t k0 = 0, uint64_t k1 = 0);

	void Clear(uint64_t k0 = 0, uint64_t k1 = 0);

    void Update(const std::string& x);

    void Update(const char* data, size_t size);

	template <typename CharT>
	void Update(const std::basic_string< CharT> &str) {
		Update(reinterpret_cast<const char*>(str.data()), str.size() * sizeof(CharT));
	}

    void Finalize() const;

    size_t GetHash() const;

	static uint64_t GetHash(uint64_t k0, uint64_t k1, const std::string& x);

private:
	mutable uint64_t v0_;
	mutable uint64_t v1_;
	mutable uint64_t v2_;
	mutable uint64_t v3_;
	mutable uint64_t count_;

	union {
		mutable uint64_t current_word;
		mutable uint8_t current_bytes[8]{};
	};
};

}