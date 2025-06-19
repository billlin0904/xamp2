//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/uuid.h>

namespace details {

constexpr uint8_t ParseHexDigital(const char c) {
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
		throw std::domain_error{ "invalid character in UUID"s };
	}
}

constexpr uint8_t ParseHex(std::string_view ptr, const int index) {
	return (ParseHexDigital(ptr[index]) << 4) + ParseHexDigital(ptr[index + 1]);
}

constexpr xamp::base::UuidBuffer ParseUuid(std::string_view str) {
	return {
		ParseHex(str, 0),
		ParseHex(str, 2),
		ParseHex(str, 4),
		ParseHex(str, 6),
		ParseHex(str, 9),
		ParseHex(str, 11),
		ParseHex(str, 14),
		ParseHex(str, 16),
		ParseHex(str, 19),
		ParseHex(str, 21),
		ParseHex(str, 24),
		ParseHex(str, 26),
		ParseHex(str, 28),
		ParseHex(str, 30),
		ParseHex(str, 32),
		ParseHex(str, 34)
	};
}

}

namespace uuid_literals {
	constexpr xamp::base::Uuid operator "" _uuid(const char* str, size_t N) {
		using namespace details;
		using namespace std::string_literals;

		if (N != xamp::base::kMaxUuidHexStringLength) {
			throw std::domain_error{ "String GUID of the form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX is expected"s };
		}

		if (!(str[8] == '-' && str[13] == '-' && str[18] == '-' && str[23] == '-')) {
			throw std::domain_error{ "UUID format must be XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" };
		}

		std::string_view sv(str, N);
		return ParseUuid(sv);
	}
}

#define XAMP_DECLARE_MAKE_CLASS_UUID(ClassName, UuidString) \
public:\
	static const xamp::base::Uuid & uuidof() {\
		using namespace uuid_literals;\
		static const xamp::base::Uuid id = UuidString##_uuid; \
		return id;\
	}\
	\
private:\
	static inline constexpr std::string_view ClassName##_ID = UuidString;\


#define XAMP_UUID_OF(T) T::uuidof()