#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <base/exception.h>
#include <base/id.h>

namespace xamp::base {

static uint8_t ToChar(const char digital) {
	if (digital > 47 && digital < 58)
		return digital - 48;

	if (digital > 96 && digital < 103)
		return digital - 87;

	if (digital > 64 && digital < 71)
		return digital - 55;

	throw Exception(Errors::XAMP_ERROR_UNKNOWN, "Invalid digital");
}

static uint8_t MakeHex(char a, char b) {
	return ToChar(a) * 16 + ToChar(b);
}

static std::array<uint8_t, XAMP_ID_MAX_SIZE> ParseString(const std::string_view& from_string) {
	if (from_string.length() != ID::MAX_ID_STR_LEN) {
		throw Exception(Errors::XAMP_ERROR_UNKNOWN, "Invalid UUID");
	}

	if (from_string[8] != '-'
		|| from_string[13] != '-'
		|| from_string[18] != '-'
		|| from_string[23] != '-') {
		throw std::exception("Invalid UUID");
	}
	
	std::array<uint8_t, XAMP_ID_MAX_SIZE> uuid{};

	for (auto i = 0, j = 0; i < from_string.length(); ++i) {
		if (from_string[i] == '-')
			continue;
		uuid[j] = MakeHex(from_string[i], from_string[i + 1]);
		++j;
		++i;
	}
	return uuid;
}

std::ostream &operator<<(std::ostream &s, const ID &id) {
  return s << std::hex << std::setfill('0')
    << std::setw(2) << static_cast<int32_t>(id.bytes_[0])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[1])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[2])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[3])
    << "-"                                  
    << std::setw(2) << static_cast<int32_t>(id.bytes_[4])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[5])
    << "-"                                  
    << std::setw(2) << static_cast<int32_t>(id.bytes_[6])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[7])
    << "-"                                  
    << std::setw(2) << static_cast<int32_t>(id.bytes_[8])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[9])
    << "-"                                  
    << std::setw(2) << static_cast<int32_t>(id.bytes_[10])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[11])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[12])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[13])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[14])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[15]);
}

const ID ID::INVALID_ID;

ID::ID() noexcept {
	bytes_.fill(0);
}

ID::ID(const std::array<uint8_t, XAMP_ID_MAX_SIZE> &bytes) noexcept
	: bytes_(bytes) {
}

ID::ID(const std::string_view &str) {
	bytes_.fill(0);
	if (!str.empty()) {
		bytes_ = ParseString(str);
	}
}

ID::operator std::string() const noexcept {
	if (bytes_.empty()) {
		return "";
	}
    std::ostringstream ostr;
	ostr << *this;
    return ostr.str();
}

ID ID::NewId() {
#ifdef _WIN32
	GUID new_id{};

	(void) CoCreateGuid(&new_id);

	const std::array<uint8_t, XAMP_ID_MAX_SIZE> bytes {
		static_cast<uint8_t>((new_id.Data1 >> 24) & 0xFF),
		static_cast<uint8_t>((new_id.Data1 >> 16) & 0xFF),
		static_cast<uint8_t>((new_id.Data1 >> 8) & 0xFF),
		static_cast<uint8_t>((new_id.Data1) & 0xFF),

		static_cast<uint8_t>((new_id.Data2 >> 8) & 0xFF),
		static_cast<uint8_t>((new_id.Data2) & 0xff),

		static_cast<uint8_t>((new_id.Data3 >> 8) & 0xFF),
		static_cast<uint8_t>((new_id.Data3) & 0xFF),

		new_id.Data4[0],
		new_id.Data4[1],
		new_id.Data4[2],
		new_id.Data4[3],
		new_id.Data4[4],
		new_id.Data4[5],
		new_id.Data4[6],
		new_id.Data4[7]
	};
#endif
	return ID(bytes);
}

ID ID::FromString(const std::string & str) {
	return ID(str);
}
	
}
