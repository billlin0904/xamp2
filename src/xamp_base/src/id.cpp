// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <iomanip>
#include <sstream>

#include <base/exception.h>
#include <base/dll.h>
#include <base/id.h>

namespace xamp::base {

static inline uint8_t MakeHex(char a, char b) {
    const char buffer[3] = { a, b, '\0' };
    const char* start = &buffer[0];
    char* endptr = nullptr;
    const auto result = std::strtol(start, &endptr, 16);
    if (endptr > start) {
        return static_cast<uint8_t>(result);        
    }
    throw std::invalid_argument("Invalid digital.");
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

    /*
    BOOL _GUIDFromString(LPCSTR, LPGUID);

    static auto shlwapi_dll = xamp::base::LoadModule("Shlwapi.dll");
    static const XAMP_DECLARE_DLL(_GUIDFromString) GUIDFromString(shlwapi_dll, 269);

    if (!GUIDFromString(from_string.data(), reinterpret_cast<GUID*>(uuid.data()))) {
        throw std::invalid_argument("Invalid ID.");
    }
    */

    /*    
    int32_t data1;
    int16_t data2, data3, data4a;
    char data4b[6];

    if (sscanf(from_string.data(), "%08X-%04hX-%04hX-%04hX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
        &data1, &data2, &data3, &data4a, data4b, data4b + 1, data4b + 2, data4b + 3, data4b + 4, data4b + 5) != 10) {
        throw std::invalid_argument("Invalid ID.");
    }

    memcpy(uuid.data() + 0x0, &data1, 4);
    memcpy(uuid.data() + 0x4, &data2, 2);
    memcpy(uuid.data() + 0x6, &data3, 2);
    memcpy(uuid.data() + 0x8, &data4a, 2);
    memcpy(uuid.data() + 0xA, data4b, 6);
    */

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

ID::ID(ID const& other) noexcept {
    *this = other;
}

ID::ID(std::array<uint8_t, kIdSize> const &bytes) noexcept
	: bytes_(bytes) {
}

ID& ID::operator=(ID const& other) noexcept {
    if (this != &other) {
        bytes_ = other.bytes_;
    }
    return *this;
}

ID::ID(ID&& other) noexcept {
    *this = std::move(other);
}

ID& ID::operator=(ID&& other) noexcept {
    if (this != &other) {
        bytes_ = other.bytes_;
    }
    return *this;
}

ID::ID(std::string_view const &str) {
	bytes_.fill(0);
	if (!str.empty()) {
		bytes_ = ParseString(str);
	}
}

ID::operator std::string() const {
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
