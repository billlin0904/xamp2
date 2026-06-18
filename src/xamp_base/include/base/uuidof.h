//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/uuid.h>

namespace details {

constexpr uint8_t parseHexDigital(const char c) {
	using namespace std::string_literals;

	/*return
		('0' <= c && c <= '9')
		? c - '0'
		: ('a' <= c && c <= 'f')
		? 10 + c - 'a'
		: ('A' <= c && c <= 'F')
		? 10 + c - 'A'
		:
		throw std::domain_error{ "invalid character in UUID"s };*/

	switch (c) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return static_cast<uint8_t>(c - '0');
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		return static_cast<uint8_t>(10 + (c - 'a'));
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		return static_cast<uint8_t>(10 + (c - 'A'));
	default:
		throw std::domain_error{ "invalid character in UUID" };
	}
}

constexpr uint8_t parseHex(std::string_view ptr, const int index) {
	return (parseHexDigital(ptr[index]) << 4) + parseHexDigital(ptr[index + 1]);
}

constexpr xamp::base::UuidBuffer parseUuid(std::string_view str) {
	return {
		parseHex(str, 0),
		parseHex(str, 2),
		parseHex(str, 4),
		parseHex(str, 6),
		parseHex(str, 9),
		parseHex(str, 11),
		parseHex(str, 14),
		parseHex(str, 16),
		parseHex(str, 19),
		parseHex(str, 21),
		parseHex(str, 24),
		parseHex(str, 26),
		parseHex(str, 28),
		parseHex(str, 30),
		parseHex(str, 32),
		parseHex(str, 34)
	};
}

}

namespace uuid_literals {
	constexpr xamp::base::Uuid operator "" _uuid(const char* str, size_t N) {
		using namespace details;
		using namespace std::string_literals;

		if (N != xamp::base::kMaxUuidHexStringLength) {
			throw std::domain_error{ "String GUID of the form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX is expected" };
		}

		if (!(str[8] == '-' && str[13] == '-' && str[18] == '-' && str[23] == '-')) {
			throw std::domain_error{ "UUID format must be XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" };
		}

		std::string_view sv(str, N);
		return parseUuid(sv);
	}
}

#define XAMP_DECLARE_MAKE_CLASS_UUID(ClassName, UuidString) \
public:\
	static const xamp::base::Uuid & uuidof() {\
		using namespace uuid_literals;\
		static constexpr xamp::base::Uuid id = UuidString##_uuid; \
		return id;\
	}\
	\
private:\
	static inline constexpr std::string_view ClassName##_ID = UuidString;\


#define XAMP_UUID_OF(t) t::uuidof()