#include <iomanip>
#include <sstream>

#include <base/exception.h>
#include <base/id.h>

namespace xamp::base {

static inline int32_t ToChar(const char C) {
    const char buffer[2] = { C, '\0' };
    const auto result = atoi(buffer);
    if (errno == ERANGE) {
        throw std::invalid_argument("Invalid digital.");
    }
    return result;
}

static inline uint8_t MakeHex(char a, char b) {
    return static_cast<uint8_t>(ToChar(a) * 16 + ToChar(b));
}

static std::array<uint8_t, kIdSize> ParseString(std::string_view const & from_string) {
    if (from_string.length() != kMaxIdStrLen) {
        throw std::invalid_argument("Invalid ID.");
	}

	if (from_string[8] != '-'
		|| from_string[13] != '-'
		|| from_string[18] != '-'
		|| from_string[23] != '-') {
        throw std::invalid_argument("Invalid ID.");
	}
	
	std::array<uint8_t, kIdSize> uuid{};

    for (size_t i = 0, j = 0; i < from_string.length(); ++i) {
		if (from_string[i] == '-')
			continue;
		uuid[j] = MakeHex(from_string[i], from_string[i + 1]);
		++j;
		++i;
	}
	return uuid;
}

std::ostream &operator<<(std::ostream &s, ID const &id) {
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

ID const ID::INVALID_ID;

ID::ID() noexcept {
	bytes_.fill(0);
}

ID::ID(std::array<uint8_t, kIdSize> const &bytes) noexcept
	: bytes_(bytes) {
}

ID::ID(std::string_view const &str) {
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

ID ID::FromString(std::string const & str) {
	return ID(str);
}
	
}
