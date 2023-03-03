//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/uuid.h>

namespace details {

constexpr uint8_t ParseHexDigital(const char c) {
	using namespace std::string_literals;

	return
		('0' <= c && c <= '9')
		? c - '0'
		: ('a' <= c && c <= 'f')
		? 10 + c - 'a'
		: ('A' <= c && c <= 'F')
		? 10 + c - 'A'
		:
		throw std::domain_error{ "invalid character in UUID"s };
}

constexpr uint8_t ParseHex(const char* ptr, const int index) {
	return (ParseHexDigital(ptr[index]) << 4) + ParseHexDigital(ptr[index + 1]);
}

constexpr xamp::base::UuidBuffer ParseUuid(const char* str) {
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

namespace UuidLiterals {
	constexpr xamp::base::Uuid operator "" _uuid(const char* str, size_t N) {
		using namespace details;
		using namespace std::string_literals;

		if (N != xamp::base::kMaxUuidHexStringLength) {
			throw std::domain_error{ "String GUID of the form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX is expected"s };
		}

		return ParseUuid(str);
	}
}

#define XAMP_DECLARE_MAKE_CLASS_UUID(ClassName, UuidString) \
public:\
	static const xamp::base::Uuid & uuidof() {\
		using namespace UuidLiterals;\
		static const xamp::base::Uuid id = UuidString##_uuid; \
		return id;\
	}\
	\
private:\
	static inline constexpr std::string_view ClassName##_ID = UuidString;\

#define XAMP_UUID_OF(T) T::uuidof()